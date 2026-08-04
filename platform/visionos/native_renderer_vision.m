#import "CTRVisionBridge.h"

extern "C" {
#include <macros.h>
#include <platform.h>
#include <platform/native_gpu.h>
#include <platform/native_log.h>
#include <platform/native_renderer.h>
}

#import <Metal/Metal.h>
#import <simd/simd.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
extern u32 CTR_MainStep(void);

int g_cfg_bilinearFiltering = 0;
int g_dbg_texturelessMode = 0;
int g_dbg_wireframeMode = 0;
int g_windowHeight = 0;
int g_windowWidth = 0;
}

#define CTR_VISION_OUTPUT_RING 3
#define CTR_VISION_VRAM_TEXTURE 1u
#define CTR_VISION_WHITE_TEXTURE 2u

struct CTRVisionUniforms
{
	matrix_float4x4 projection;
	int textureFormat;
	int semiTransPass;
	int drawMaskSet;
	int textureOutputSTP;
	int renderLayer;
};

static id<MTLDevice> s_device;
static id<MTLCommandQueue> s_queue;
static id<MTLLibrary> s_library;
static id<MTLFunction> s_vertexFunction;
static id<MTLFunction> s_fragmentFunction;
static id<MTLTexture> s_vramTexture;
static id<MTLTexture> s_sceneTextures[CTR_VISION_OUTPUT_RING];
static id<MTLTexture> s_hudTextures[CTR_VISION_OUTPUT_RING];
static id<MTLTexture> s_stencilTextures[CTR_VISION_OUTPUT_RING][NATIVE_VISION_LAYER_COUNT];
static id<MTLRenderPipelineState> s_pipelines[5][5];
static id<MTLDepthStencilState> s_depthStencilDraw;
static id<MTLDepthStencilState> s_depthStencilTest;
static id<MTLCommandBuffer> s_commandBuffer;
static id<MTLRenderCommandEncoder> s_encoder;
static id<MTLBuffer> s_vertexBuffer;
static NSLock *s_frameLock;
static int s_writeIndex;
static int s_nextWriteIndex;
static int s_publishedIndex;
static uint64_t s_publishedSerial;
static BOOL s_publishedHasHud;
static int s_outputWidth = 512;
static int s_outputHeight = 216;
static int s_resolutionScale = 3;
static int s_requestedResolutionScale = 3;
static int s_currentLayer;
static int s_currentTextureFormat;
static int s_currentSemiTransPass;
static int s_currentMaskSet;
static int s_currentTextureOutputSTP;
static int s_currentStencilMode;
static BlendMode s_currentBlendMode;
static matrix_float4x4 s_projection;
static MTLScissorRect s_scissor;
static int s_scissorEnabled;
static BOOL s_layerStarted[NATIVE_VISION_LAYER_COUNT];
static MTLClearColor s_worldClearColor;
static u16 s_vramPixels[VRAM_WIDTH * VRAM_HEIGHT];
static BOOL s_vramDirty;

static const char *s_shaderSourceUTF8 = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct VertexIn {
  short4 position [[attribute(0)]];
  uchar4 texcoord [[attribute(1)]];
  float4 color [[attribute(2)]];
  char4 extra [[attribute(3)]];
};

struct Uniforms {
  float4x4 projection;
  int textureFormat;
  int semiTransPass;
  int drawMaskSet;
  int textureOutputSTP;
  int renderLayer;
};

struct VertexOut {
  float4 position [[position]];
  float2 uv;
  float brightness;
  float dither;
  float4 color;
  float2 pageClut [[flat]];
  float2 ditherCoord;
};

vertex VertexOut ctrVertex(VertexIn in [[stage_in]], constant Uniforms &u [[buffer(1)]]) {
  VertexOut out;
  float2 p = float2(in.position.xy);
  out.position = u.projection * float4(p, 0.0, 1.0);
  out.uv = float2(in.texcoord.xy) + float2(in.extra.xy) * 0.5;
  // PsyCross stores the texture modulation multiplier directly (normally 2),
  // while vertex color is the normalized 0...1 value. Match its GL shader.
  out.brightness = float(in.texcoord.z);
  out.dither = float(in.texcoord.w);
  out.color = in.color;
  out.pageClut = float2(in.position.zw);
  out.ditherCoord = p;
  return out;
}

static uint packedVRAM(texture2d<uint, access::read> vram, int2 p) {
  p.x = clamp(p.x, 0, 1023);
  p.y = clamp(p.y, 0, 511);
  uint4 bytes = vram.read(uint2(p));
  return bytes.r | (bytes.g << 8);
}

static float4 decode5551(uint pixel) {
  uint3 c5 = uint3(pixel & 31u, (pixel >> 5) & 31u, (pixel >> 10) & 31u);
  uint3 c8 = (c5 << 3) | (c5 >> 2);
  return float4(float3(c8) / 255.0, float((pixel >> 15) & 1u));
}

fragment float4 ctrFragment(VertexOut in [[stage_in]],
                            constant Uniforms &u [[buffer(1)]],
                            texture2d<uint, access::read> vram [[texture(0)]]) {
  float4 texel = float4(1.0);
  float stp = 0.0;
  if (u.textureFormat >= 0 && u.textureFormat <= 2) {
    int page = int(in.pageClut.x);
    int clut = int(in.pageClut.y);
    int pageX = (page & 15) * 64;
    int pageY = (page >> 4) * 256;
    int2 uv = int2(floor(in.uv));
    uint pixel = 0;
    if (u.textureFormat == 0) {
      uint packed = packedVRAM(vram, int2(pageX + uv.x / 4, pageY + uv.y));
      uint index = (packed >> ((uv.x & 3) * 4)) & 15u;
      pixel = packedVRAM(vram, int2((clut & 63) * 16 + int(index), clut >> 6));
    } else if (u.textureFormat == 1) {
      uint packed = packedVRAM(vram, int2(pageX + uv.x / 2, pageY + uv.y));
      uint index = (packed >> ((uv.x & 1) * 8)) & 255u;
      pixel = packedVRAM(vram, int2((clut & 63) * 16 + int(index), clut >> 6));
    } else {
      pixel = packedVRAM(vram, int2(pageX + uv.x, pageY + uv.y));
    }
    if ((pixel & 0x7fffu) == 0u) discard_fragment();
    texel = decode5551(pixel);
    stp = texel.a;
    if (u.semiTransPass == 1 && stp >= 0.5) discard_fragment();
    if (u.semiTransPass == 2 && stp < 0.5) discard_fragment();
  }

  float4 result = texel * in.color;
  result.rgb *= in.brightness;
  constexpr float dither[16] = {-4,0,-3,1, 2,-2,3,-1, -3,1,-4,0, 3,-1,2,-2};
  int2 dc = int2(floor(in.ditherCoord)) & 3;
  result.rgb += (dither[dc.x * 4 + dc.y] / 255.0) * in.dither;
  result.a = u.renderLayer == 2
    ? 1.0
    : ((u.drawMaskSet != 0 || (u.textureOutputSTP != 0 && stp >= 0.5)) ? 1.0 : 0.0);
  return saturate(result);
}
)MSL";

@implementation CTRVisionFrameHub
+ (void)load
{
	s_frameLock = [[NSLock alloc] init];
}
+ (id<MTLDevice>)device
{
	[s_frameLock lock];
	id<MTLDevice> device = s_device;
	[s_frameLock unlock];
	return device;
}
+ (id<MTLTexture>)currentSceneTexture
{
	[s_frameLock lock];
	id<MTLTexture> texture = s_publishedSerial != 0 ? s_sceneTextures[s_publishedIndex] : nil;
	[s_frameLock unlock];
	return texture;
}
+ (id<MTLTexture>)currentHudTexture
{
	[s_frameLock lock];
	id<MTLTexture> texture = (s_publishedSerial != 0) && s_publishedHasHud ? s_hudTextures[s_publishedIndex] : nil;
	[s_frameLock unlock];
	return texture;
}
+ (uint64_t)serial
{
	[s_frameLock lock];
	uint64_t serial = s_publishedSerial;
	[s_frameLock unlock];
	return serial;
}
@end

extern "C" {

int NativeVision_RunMainStepWithAutoreleasePool(void)
{
	@autoreleasepool
	{
		return (int)CTR_MainStep();
	}
}

static void CTRVision_EndEncoder(void)
{
	if (s_encoder != nil)
	{
		[s_encoder endEncoding];
		s_encoder = nil;
	}
}

static void CTRVision_EnsureOutputs(void)
{
	int width = MAX(1, s_outputWidth * s_resolutionScale);
	int height = MAX(1, s_outputHeight * s_resolutionScale);
	[s_frameLock lock];
	if ((s_sceneTextures[0] != nil) && ((int)s_sceneTextures[0].width == width) && ((int)s_sceneTextures[0].height == height))
	{
		[s_frameLock unlock];
		return;
	}

	CTRVision_EndEncoder();
	s_publishedSerial = 0;
	s_publishedHasHud = NO;
	for (int i = 0; i < CTR_VISION_OUTPUT_RING; i++)
	{
		MTLTextureDescriptor *scene = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm_sRGB
		                                                                                 width:width height:height mipmapped:NO];
		scene.textureType = MTLTextureType2DArray;
		scene.arrayLength = 2;
		scene.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
		scene.storageMode = MTLStorageModePrivate;
		s_sceneTextures[i] = [s_device newTextureWithDescriptor:scene];
		s_sceneTextures[i].label = @"CTR stereo scene";

		MTLTextureDescriptor *hud = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm_sRGB
		                                                                               width:width height:height mipmapped:NO];
		hud.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
		hud.storageMode = MTLStorageModePrivate;
		s_hudTextures[i] = [s_device newTextureWithDescriptor:hud];
		s_hudTextures[i].label = @"CTR HUD";

		for (int layer = 0; layer < NATIVE_VISION_LAYER_COUNT; layer++)
		{
			MTLTextureDescriptor *stencil = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatStencil8
			                                                                                         width:width height:height mipmapped:NO];
			stencil.usage = MTLTextureUsageRenderTarget;
			stencil.storageMode = MTLStorageModePrivate;
			s_stencilTextures[i][layer] = [s_device newTextureWithDescriptor:stencil];
		}
	}
	[s_frameLock unlock];
	s_scissor = (MTLScissorRect){0, 0, (NSUInteger)width, (NSUInteger)height};
}

static id<MTLRenderPipelineState> CTRVision_Pipeline(TexFormat format, BlendMode blend)
{
	int formatIndex = (format >= TF_4_BIT && format <= TF_32_BIT_RGBA) ? (int)format : 4;
	int blendIndex = (blend >= BM_NONE && blend <= BM_ADD_QUATER_SOURCE) ? (int)blend : 0;
	if (s_pipelines[formatIndex][blendIndex] != nil)
	{
		return s_pipelines[formatIndex][blendIndex];
	}

	MTLRenderPipelineDescriptor *desc = [[MTLRenderPipelineDescriptor alloc] init];
	desc.label = @"CTR PS1 primitives";
	desc.vertexFunction = s_vertexFunction;
	desc.fragmentFunction = s_fragmentFunction;
	desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
	desc.stencilAttachmentPixelFormat = MTLPixelFormatStencil8;

	MTLVertexDescriptor *vertex = [[MTLVertexDescriptor alloc] init];
	vertex.attributes[0].format = MTLVertexFormatShort4;
	vertex.attributes[0].offset = offsetof(GrVertex, x);
	vertex.attributes[0].bufferIndex = 0;
	vertex.attributes[1].format = MTLVertexFormatUChar4;
	vertex.attributes[1].offset = offsetof(GrVertex, u);
	vertex.attributes[1].bufferIndex = 0;
	vertex.attributes[2].format = MTLVertexFormatUChar4Normalized;
	vertex.attributes[2].offset = offsetof(GrVertex, r);
	vertex.attributes[2].bufferIndex = 0;
	vertex.attributes[3].format = MTLVertexFormatChar4;
	vertex.attributes[3].offset = offsetof(GrVertex, tcx);
	vertex.attributes[3].bufferIndex = 0;
	vertex.layouts[0].stride = sizeof(GrVertex);
	vertex.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
	desc.vertexDescriptor = vertex;

	MTLRenderPipelineColorAttachmentDescriptor *color = desc.colorAttachments[0];
	if (blend != BM_NONE)
	{
		color.blendingEnabled = YES;
		color.sourceAlphaBlendFactor = MTLBlendFactorOne;
		color.destinationAlphaBlendFactor = MTLBlendFactorZero;
		switch (blend)
		{
		case BM_AVERAGE:
			color.sourceRGBBlendFactor = MTLBlendFactorBlendColor;
			color.destinationRGBBlendFactor = MTLBlendFactorOneMinusBlendColor;
			break;
		case BM_ADD:
			color.sourceRGBBlendFactor = MTLBlendFactorOne;
			color.destinationRGBBlendFactor = MTLBlendFactorOne;
			break;
		case BM_SUBTRACT:
			color.rgbBlendOperation = MTLBlendOperationReverseSubtract;
			color.sourceRGBBlendFactor = MTLBlendFactorOne;
			color.destinationRGBBlendFactor = MTLBlendFactorOne;
			break;
		case BM_ADD_QUATER_SOURCE:
			color.sourceRGBBlendFactor = MTLBlendFactorBlendColor;
			color.destinationRGBBlendFactor = MTLBlendFactorOne;
			break;
		default:
			break;
		}
	}

	NSError *error = nil;
	s_pipelines[formatIndex][blendIndex] = [s_device newRenderPipelineStateWithDescriptor:desc error:&error];
	if (s_pipelines[formatIndex][blendIndex] == nil)
	{
		Platform_LogError("[CTR Vision] Metal pipeline failed: %s\n", error.localizedDescription.UTF8String);
	}
	return s_pipelines[formatIndex][blendIndex];
}

static id<MTLRenderCommandEncoder> CTRVision_Encoder(void)
{
	if (s_encoder != nil)
	{
		return s_encoder;
	}
	CTRVision_EnsureOutputs();

	MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
	if (s_currentLayer == NATIVE_VISION_LAYER_HUD)
	{
		pass.colorAttachments[0].texture = s_hudTextures[s_writeIndex];
	}
	else
	{
		pass.colorAttachments[0].texture = s_sceneTextures[s_writeIndex];
		pass.colorAttachments[0].slice = (NSUInteger)s_currentLayer;
	}
	pass.colorAttachments[0].loadAction = s_layerStarted[s_currentLayer] ? MTLLoadActionLoad : MTLLoadActionClear;
	pass.colorAttachments[0].storeAction = MTLStoreActionStore;
	pass.colorAttachments[0].clearColor = s_currentLayer == NATIVE_VISION_LAYER_HUD
	                                           ? MTLClearColorMake(0, 0, 0, 0)
	                                           : s_worldClearColor;
	pass.stencilAttachment.texture = s_stencilTextures[s_writeIndex][s_currentLayer];
	pass.stencilAttachment.loadAction = s_layerStarted[s_currentLayer] ? MTLLoadActionLoad : MTLLoadActionClear;
	pass.stencilAttachment.storeAction = MTLStoreActionStore;
	pass.stencilAttachment.clearStencil = 0;
	s_layerStarted[s_currentLayer] = YES;

	s_encoder = [s_commandBuffer renderCommandEncoderWithDescriptor:pass];
	s_encoder.label = @"CTR eye layer";
	[s_encoder setViewport:(MTLViewport){0, 0, (double)(s_outputWidth * s_resolutionScale),
	                                     (double)(s_outputHeight * s_resolutionScale), 0, 1}];
	return s_encoder;
}

int NativeRenderer_InitialiseRender(char *windowName, int width, int height, int fullscreen)
{
	(void)windowName;
	(void)fullscreen;
	g_windowWidth = width;
	g_windowHeight = height;
	id<MTLDevice> device = MTLCreateSystemDefaultDevice();
	id<MTLCommandQueue> queue = [device newCommandQueue];
	if ((device == nil) || (queue == nil))
	{
		return 0;
	}
	[s_frameLock lock];
	s_device = device;
	s_queue = queue;
	[s_frameLock unlock];

	NSError *error = nil;
	NSString *shaderSource = [NSString stringWithUTF8String:s_shaderSourceUTF8];
	s_library = [s_device newLibraryWithSource:shaderSource options:nil error:&error];
	if (s_library == nil)
	{
		Platform_LogError("[CTR Vision] Metal shader failed: %s\n", error.localizedDescription.UTF8String);
		return 0;
	}
	s_vertexFunction = [s_library newFunctionWithName:@"ctrVertex"];
	s_fragmentFunction = [s_library newFunctionWithName:@"ctrFragment"];

	MTLStencilDescriptor *drawStencil = [[MTLStencilDescriptor alloc] init];
	drawStencil.stencilCompareFunction = MTLCompareFunctionAlways;
	drawStencil.stencilFailureOperation = MTLStencilOperationReplace;
	drawStencil.depthFailureOperation = MTLStencilOperationReplace;
	drawStencil.depthStencilPassOperation = MTLStencilOperationReplace;
	MTLDepthStencilDescriptor *draw = [[MTLDepthStencilDescriptor alloc] init];
	draw.frontFaceStencil = drawStencil;
	draw.backFaceStencil = drawStencil;
	s_depthStencilDraw = [s_device newDepthStencilStateWithDescriptor:draw];

	MTLStencilDescriptor *testStencil = [[MTLStencilDescriptor alloc] init];
	testStencil.stencilCompareFunction = MTLCompareFunctionNotEqual;
	testStencil.stencilFailureOperation = MTLStencilOperationKeep;
	testStencil.depthFailureOperation = MTLStencilOperationKeep;
	testStencil.depthStencilPassOperation = MTLStencilOperationKeep;
	MTLDepthStencilDescriptor *test = [[MTLDepthStencilDescriptor alloc] init];
	test.frontFaceStencil = testStencil;
	test.backFaceStencil = testStencil;
	s_depthStencilTest = [s_device newDepthStencilStateWithDescriptor:test];
	return 1;
}

int NativeRenderer_InitialisePSX(void)
{
	memset(s_vramPixels, 0, sizeof(s_vramPixels));
	MTLTextureDescriptor *desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRG8Uint
	                                                                            width:VRAM_WIDTH height:VRAM_HEIGHT mipmapped:NO];
	desc.usage = MTLTextureUsageShaderRead;
	desc.storageMode = MTLStorageModeShared;
	s_vramTexture = [s_device newTextureWithDescriptor:desc];
	s_vramTexture.label = @"CTR PS1 VRAM";
	s_vramDirty = YES;
	CTRVision_EnsureOutputs();
	Platform_Log("[CTR Vision] internal resolution initialized at %dx (%dx%d)\n", s_resolutionScale,
	             s_outputWidth * s_resolutionScale, s_outputHeight * s_resolutionScale);
	return s_vramTexture != nil;
}

void NativeRenderer_Shutdown(void)
{
	CTRVision_EndEncoder();
	s_commandBuffer = nil;
	s_vertexBuffer = nil;
	s_vramTexture = nil;
	[s_frameLock lock];
	for (int i = 0; i < CTR_VISION_OUTPUT_RING; i++)
	{
		s_sceneTextures[i] = nil;
		s_hudTextures[i] = nil;
	}
	s_publishedSerial = 0;
	s_publishedHasHud = NO;
	s_device = nil;
	s_queue = nil;
	[s_frameLock unlock];
}

void NativeRenderer_ResetDevice(void) {}
void NativeRenderer_UpdateSwapIntervalState(int swapInterval) { (void)swapInterval; }
int NativeRenderer_SetInternalResolutionScale(int scale)
{
	int requestedScale = MAX(1, MIN(scale, 4));
	[s_frameLock lock];
	s_requestedResolutionScale = requestedScale;
	[s_frameLock unlock];
	return requestedScale;
}

void NativeRenderer_BeginScene(void)
{
	CTRVision_EndEncoder();
	[s_frameLock lock];
	int requestedResolutionScale = s_requestedResolutionScale;
	[s_frameLock unlock];
	if (s_resolutionScale != requestedResolutionScale)
	{
		s_resolutionScale = requestedResolutionScale;
		CTRVision_EnsureOutputs();
		Platform_Log("[CTR Vision] internal resolution changed to %dx (%dx%d)\n", s_resolutionScale,
		             s_outputWidth * s_resolutionScale, s_outputHeight * s_resolutionScale);
	}
	s_writeIndex = s_nextWriteIndex;
	s_nextWriteIndex = (s_nextWriteIndex + 1) % CTR_VISION_OUTPUT_RING;
	s_commandBuffer = [s_queue commandBuffer];
	s_commandBuffer.label = @"CTR native frame";
	s_currentLayer = NATIVE_VISION_LAYER_LEFT;
	s_currentBlendMode = BM_NONE;
	s_currentTextureFormat = TF_16_BIT;
	s_currentSemiTransPass = 0;
	s_currentMaskSet = 0;
	s_currentTextureOutputSTP = 0;
	s_currentStencilMode = 0;
	s_scissorEnabled = 0;
	memset(s_layerStarted, 0, sizeof(s_layerStarted));
	s_worldClearColor = MTLClearColorMake(0, 0, 0, 1);
	if (activeDispEnv.disp.w > 0 && activeDispEnv.disp.h > 0)
	{
		s_outputWidth = activeDispEnv.disp.w;
		s_outputHeight = activeDispEnv.disp.h;
		CTRVision_EnsureOutputs();
	}
	if (s_vramDirty)
	{
		[s_vramTexture replaceRegion:MTLRegionMake2D(0, 0, VRAM_WIDTH, VRAM_HEIGHT) mipmapLevel:0
		                    withBytes:s_vramPixels bytesPerRow:VRAM_WIDTH * sizeof(u16)];
		s_vramDirty = NO;
	}
}

void NativeRenderer_EndScene(void)
{
	CTRVision_EndEncoder();
	int layerCount = NativeVision_GetMode() == NATIVE_VISION_MODE_WINDOW ? 1 : NATIVE_VISION_LAYER_COUNT;
	for (int layer = 0; layer < layerCount; layer++)
	{
		if (!s_layerStarted[layer])
		{
			s_currentLayer = layer;
			(void)CTRVision_Encoder();
			CTRVision_EndEncoder();
		}
	}
}
void NativeRenderer_EndGpuFrame(void) {}
void NativeRenderer_FinishGpuMeasurements(void) {}

void NativeRenderer_SwapWindow(void)
{
	CTRVision_EndEncoder();
	if (s_commandBuffer == nil)
	{
		return;
	}
	int completedIndex = s_writeIndex;
	BOOL completedHasHud = s_layerStarted[NATIVE_VISION_LAYER_HUD];
	[s_commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
		if (buffer.status == MTLCommandBufferStatusCompleted)
		{
			[s_frameLock lock];
			s_publishedIndex = completedIndex;
			s_publishedHasHud = completedHasHud;
			s_publishedSerial++;
			[s_frameLock unlock];
		}
	}];
	[s_commandBuffer commit];
	s_commandBuffer = nil;
}

TextureID NativeRenderer_GetVRAMTexture(void) { return CTR_VISION_VRAM_TEXTURE; }
TextureID NativeRenderer_GetWhiteTexture(void) { return CTR_VISION_WHITE_TEXTURE; }
void NativeRenderer_SetTexture(TextureID texture, TexFormat format)
{
	s_currentTextureFormat = texture == CTR_VISION_WHITE_TEXTURE ? -1 : (int)format;
}
void NativeRenderer_SetOverrideTextureSize(int width, int height) { (void)width; (void)height; }
void NativeRenderer_SetPSXTextureSemiTransPass(int pass) { s_currentSemiTransPass = pass; }
void NativeRenderer_SetPSXFramebufferFetchBlendMode(int blendMode) { (void)blendMode; }
int NativeRenderer_UsesFramebufferFetch(void) { return 0; }
void NativeRenderer_SetPSXTextureOutputSTP(int enabled) { s_currentTextureOutputSTP = enabled; }
void NativeRenderer_SetPSXDrawMaskSet(int maskSet) { s_currentMaskSet = maskSet; }

void NativeRenderer_SetVisionLayer(int layer)
{
	if ((layer < 0) || (layer >= NATIVE_VISION_LAYER_COUNT) || (layer == s_currentLayer))
	{
		return;
	}
	CTRVision_EndEncoder();
	s_currentLayer = layer;
}

void NativeRenderer_SetStencilMode(int drawPrim) { s_currentStencilMode = drawPrim; }
void NativeRenderer_SetBlendMode(BlendMode blendMode) { s_currentBlendMode = blendMode; }
void NativeRenderer_SetOffscreenState(const RECT16 *rect, int enable) { (void)rect; (void)enable; }

void NativeRenderer_SetProjection(const RECT16 *drawRect, const DISPENV *displayEnv, int offscreen)
{
	int width = offscreen ? drawRect->w : MAX(1, displayEnv->disp.w);
	int height = offscreen ? drawRect->h : MAX(1, displayEnv->disp.h);
	float left = 0.0f, right = (float)width, bottom = (float)height, top = 0.0f;
	float a = 2.0f / (right - left);
	float b = 2.0f / (top - bottom);
	float x = (left + right) / (left - right);
	float y = (bottom + top) / (bottom - top);
	s_projection = matrix_identity_float4x4;
	s_projection.columns[0] = (vector_float4){a, 0, 0, 0};
	s_projection.columns[1] = (vector_float4){0, b, 0, 0};
	s_projection.columns[2] = (vector_float4){0, 0, -1, 0};
	s_projection.columns[3] = (vector_float4){x, y, 0, 1};
}

void NativeRenderer_SetupClipMode(const RECT16 *rect, const DISPENV *displayEnv, int enable)
{
	s_scissorEnabled = enable && rect->w > 0 && rect->h > 0;
	if (!s_scissorEnabled)
	{
		return;
	}
	int x = MAX(0, rect->x - displayEnv->disp.x) * s_resolutionScale;
	int y = MAX(0, rect->y - displayEnv->disp.y) * s_resolutionScale;
	int w = MIN(rect->w * s_resolutionScale, s_outputWidth * s_resolutionScale - x);
	int h = MIN(rect->h * s_resolutionScale, s_outputHeight * s_resolutionScale - y);
	s_scissor = (MTLScissorRect){(NSUInteger)x, (NSUInteger)y, (NSUInteger)MAX(0, w), (NSUInteger)MAX(0, h)};
}

void NativeRenderer_UpdateVertexBuffer(const GrVertex *vertices, int count)
{
	if ((vertices == NULL) || (count <= 0))
	{
		s_vertexBuffer = nil;
		return;
	}
	s_vertexBuffer = [s_device newBufferWithBytes:vertices length:(NSUInteger)count * sizeof(GrVertex) options:MTLResourceStorageModeShared];
}

void NativeRenderer_DrawTriangles(int startVertex, int triangles)
{
	if ((s_vertexBuffer == nil) || (triangles <= 0))
	{
		return;
	}
	id<MTLRenderCommandEncoder> encoder = CTRVision_Encoder();
	id<MTLRenderPipelineState> pipeline = CTRVision_Pipeline((TexFormat)s_currentTextureFormat, s_currentBlendMode);
	if ((encoder == nil) || (pipeline == nil))
	{
		return;
	}
	[encoder setRenderPipelineState:pipeline];
	[encoder setVertexBuffer:s_vertexBuffer offset:0 atIndex:0];
	struct CTRVisionUniforms uniforms = {s_projection, s_currentTextureFormat, s_currentSemiTransPass,
	                                    s_currentMaskSet, s_currentTextureOutputSTP, s_currentLayer};
	[encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
	[encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:1];
	[encoder setFragmentTexture:s_vramTexture atIndex:0];
	[encoder setDepthStencilState:s_currentStencilMode ? s_depthStencilDraw : s_depthStencilTest];
	[encoder setStencilReferenceValue:1];
	if (s_currentBlendMode == BM_AVERAGE)
	{
		[encoder setBlendColorRed:0.5 green:0.5 blue:0.5 alpha:0.5];
	}
	else if (s_currentBlendMode == BM_ADD_QUATER_SOURCE)
	{
		[encoder setBlendColorRed:0.25 green:0.25 blue:0.25 alpha:0.25];
	}
	if (s_scissorEnabled && s_scissor.width > 0 && s_scissor.height > 0)
	{
		[encoder setScissorRect:s_scissor];
	}
	else
	{
		[encoder setScissorRect:(MTLScissorRect){0, 0, (NSUInteger)(s_outputWidth * s_resolutionScale),
		                                                 (NSUInteger)(s_outputHeight * s_resolutionScale)}];
	}
	[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:(NSUInteger)startVertex vertexCount:(NSUInteger)triangles * 3];
}

void NativeRenderer_Clear(int x, int y, int w, int h, u8 r, u8 g, u8 b)
{
	(void)x; (void)y; (void)w; (void)h;
	s_worldClearColor = MTLClearColorMake((double)((r >> 3) << 3) / 255.0,
	                                      (double)((g >> 3) << 3) / 255.0,
	                                      (double)((b >> 3) << 3) / 255.0, 1.0);
}

void NativeRenderer_ClearVRAM(int x, int y, int w, int h, u8 r, u8 g, u8 b)
{
	u16 color = (u16)((r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10));
	for (int row = MAX(0, y); row < MIN(VRAM_HEIGHT, y + h); row++)
	{
		for (int col = MAX(0, x); col < MIN(VRAM_WIDTH, x + w); col++)
		{
			s_vramPixels[row * VRAM_WIDTH + col] = color;
		}
	}
	s_vramDirty = YES;
}

void NativeRenderer_CopyVRAM(u16 *src, int x, int y, int w, int h, int dstX, int dstY)
{
	if ((w <= 0) || (h <= 0)) return;
	if (src == NULL)
	{
		u16 *temporary = (u16 *)malloc((size_t)w * h * sizeof(u16));
		if (temporary == NULL) return;
		for (int row = 0; row < h; row++) memcpy(temporary + row * w, s_vramPixels + (y + row) * VRAM_WIDTH + x, (size_t)w * sizeof(u16));
		for (int row = 0; row < h; row++) memcpy(s_vramPixels + (dstY + row) * VRAM_WIDTH + dstX, temporary + row * w, (size_t)w * sizeof(u16));
		free(temporary);
	}
	else
	{
		for (int row = 0; row < h; row++) memcpy(s_vramPixels + (dstY + row) * VRAM_WIDTH + dstX, src + (y + row) * w + x, (size_t)w * sizeof(u16));
	}
	s_vramDirty = YES;
}

void NativeRenderer_ReadVRAM(u16 *dst, int x, int y, int w, int h)
{
	if (dst == NULL) return;
	for (int row = 0; row < h; row++) memcpy(dst + row * w, s_vramPixels + (y + row) * VRAM_WIDTH + x, (size_t)w * sizeof(u16));
}
void NativeRenderer_UpdateVRAM(void)
{
	if (s_vramDirty && s_vramTexture != nil)
	{
		[s_vramTexture replaceRegion:MTLRegionMake2D(0, 0, VRAM_WIDTH, VRAM_HEIGHT) mipmapLevel:0 withBytes:s_vramPixels bytesPerRow:VRAM_WIDTH * sizeof(u16)];
		s_vramDirty = NO;
	}
}
int NativeRenderer_GetVRAMStateSize(void) { return (int)sizeof(s_vramPixels); }
int NativeRenderer_CaptureVRAMState(void *dst, int size)
{
	if ((dst == NULL) || (size < (int)sizeof(s_vramPixels))) return 0;
	memcpy(dst, s_vramPixels, sizeof(s_vramPixels));
	return 1;
}
int NativeRenderer_RestoreVRAMState(const void *src, int size)
{
	if ((src == NULL) || (size < (int)sizeof(s_vramPixels))) return 0;
	memcpy(s_vramPixels, src, sizeof(s_vramPixels));
	s_vramDirty = YES;
	return 1;
}

void NativeRenderer_StoreFrameBuffer(int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; }
void NativeRenderer_PresentMainRenderTarget(void) {}
void NativeRenderer_PresentVRAMRect(int x, int y, int w, int h)
{
	NativeRenderer_UpdateVRAM();
	if ((s_commandBuffer == nil) || (w <= 0) || (h <= 0))
	{
		return;
	}

	CTRVision_EndEncoder();
	CTRVision_EnsureOutputs();
	const int outputWidth = s_outputWidth * s_resolutionScale;
	const int outputHeight = s_outputHeight * s_resolutionScale;
	u32 *pixels = (u32 *)malloc((size_t)outputWidth * outputHeight * sizeof(*pixels));
	if (pixels == NULL)
	{
		return;
	}
	for (int outputY = 0; outputY < outputHeight; outputY++)
	{
		int sourceY = MAX(0, MIN(VRAM_HEIGHT - 1, y + (outputY * h) / outputHeight));
		for (int outputX = 0; outputX < outputWidth; outputX++)
		{
			int sourceX = MAX(0, MIN(VRAM_WIDTH - 1, x + (outputX * w) / outputWidth));
			u16 pixel = s_vramPixels[sourceY * VRAM_WIDTH + sourceX];
			u32 red = (u32)(pixel & 31u);
			u32 green = (u32)((pixel >> 5) & 31u);
			u32 blue = (u32)((pixel >> 10) & 31u);
			red = (red << 3) | (red >> 2);
			green = (green << 3) | (green >> 2);
			blue = (blue << 3) | (blue >> 2);
			pixels[outputY * outputWidth + outputX] = 0xff000000u | (red << 16) | (green << 8) | blue;
		}
	}

	id<MTLBuffer> upload = [s_device newBufferWithBytes:pixels
	                                           length:(NSUInteger)outputWidth * outputHeight * sizeof(*pixels)
	                                          options:MTLResourceStorageModeShared];
	free(pixels);
	if (upload == nil)
	{
		return;
	}
	id<MTLBlitCommandEncoder> blit = [s_commandBuffer blitCommandEncoder];
	/* Keep both slices coherent across a window/immersive transition. */
	for (int slice = 0; slice < 2; slice++)
	{
		[blit copyFromBuffer:upload sourceOffset:0 sourceBytesPerRow:(NSUInteger)outputWidth * sizeof(u32)
		 sourceBytesPerImage:(NSUInteger)outputWidth * outputHeight * sizeof(u32)
		          sourceSize:MTLSizeMake((NSUInteger)outputWidth, (NSUInteger)outputHeight, 1)
		           toTexture:s_sceneTextures[s_writeIndex] destinationSlice:(NSUInteger)slice destinationLevel:0
		  destinationOrigin:MTLOriginMake(0, 0, 0)];
		s_layerStarted[slice] = YES;
	}
	[blit endEncoding];
}
void NativeRenderer_PresentVRAMDisplay(void) { NativeRenderer_PresentVRAMRect(activeDispEnv.disp.x, activeDispEnv.disp.y, activeDispEnv.disp.w, activeDispEnv.disp.h); }
int NativeRenderer_CapturePresentedRGBA(u8 *dst, int width, int height) { (void)dst; (void)width; (void)height; return 0; }
void NativeRenderer_SaveVRAM(const char *path, int x, int y, int w, int h, int framebuffer)
{ (void)path; (void)x; (void)y; (void)w; (void)h; (void)framebuffer; }
void NativeRenderer_PushDebugLabel(const char *label) { (void)label; }
void NativeRenderer_PopDebugLabel(void) {}
int NativeRenderer_RunDialectSelfTest(void) { return 0; }
int NativeRenderer_RunPixelSelfTest(void) { return 0; }

} /* extern "C" */

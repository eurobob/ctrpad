/*
 * Derived from REDRIVER2/PsyCross MIT source:
 * externals/PsyCross/src/render/PsyX_render.cpp
 * See THIRD_PARTY_NOTICES.md for copyright and license details.
 */

#include <macros.h>
#include <platform.h>
#include "platform/native_renderer_types.h"
#include <SDL3/SDL.h>

#include "platform/native_gpu.h"
#include "platform/native_glad.h"
#include "platform/native_log.h"
#include "platform/native_perf.h"
#include "platform/native_renderer.h"

#include <assert.h>
#include <string.h>

#ifdef _WIN32
#include "platform/native_win32.h"

__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#endif // def WIN32

#define VRAM_FORMAT          GL_RG
// NOTE(penta3): VRAM holds packed 16-bit PSX pixels as two bytes (R=low,
// G=high), uploaded as GL_RG + GL_UNSIGNED_BYTE. RG8 is the faithful storage:
// 2 bytes/texel = real PS1's 1MB VRAM, vs RG32F's 8 bytes/texel (4MB). With
// NEAREST sampling RG8 returns byte/255 exactly, identical to what the shader
// got from RG32F, so CLUT/texture-page reconstruction is unchanged.
#define VRAM_INTERNAL_FORMAT GL_RG8

extern SDL_Window *g_window;

#define NATIVE_RENDERER_LOG(fmt, ...)   Platform_Log("[CTR Renderer] " fmt, __VA_ARGS__)
#define NATIVE_RENDERER_ERROR(fmt, ...) Platform_LogError("[CTR Renderer] [%s] - " fmt, __func__, __VA_ARGS__)

#define MAX_NUM_VERTEX_BUFFERS          (2)
#define PSX_SCREEN_ASPECT               (240.0f / 320.0f) // PSX screen is mapped always to this aspect

#if defined(CTR_INTERNAL) && !defined(CTR_NATIVE_RENDERER_GLES)
#define CTR_NATIVE_GPU_TIMERS 1
#endif

#if defined(CTR_NATIVE_GPU_TIMERS)
#ifndef GL_TIME_ELAPSED
#define GL_TIME_ELAPSED 0x88BF
#endif
#define NATIVE_GPU_TIMER_QUERY_COUNT 8

struct NativeGpuTimerQuery
{
	GLuint id;
	u32 frameIndex;
	b32 pending;
};

global_variable struct NativeGpuTimerQuery s_gpuTimerQueries[NATIVE_GPU_TIMER_QUERY_COUNT];
global_variable u32 s_gpuTimerFrameIndex;
global_variable s32 s_gpuTimerNextQuery;
global_variable b32 s_gpuTimerSupported;
global_variable b32 s_gpuTimerActive;
#endif

struct NativeRendererDialect
{
	int contextMajor;
	int contextMinor;
	int contextProfile;
	const char *apiName;
	const char *profileName;
	const char *shaderName;
	const char *loaderName;
	const char *vertexShaderHeader;
	const char *fragmentShaderHeader;
	b32 wireframeEnabled;
	b32 debugLabelsEnabled;
	b32 gpuTimerEnabled;
};

#if defined(CTR_NATIVE_RENDERER_GLES)
global_variable const struct NativeRendererDialect s_rendererDialect = {
    3,
    0,
    SDL_GL_CONTEXT_PROFILE_ES,
    "gles",
    "es",
    "300es",
    "sdl-proc",
    "\t#version 300 es\n"
    "\tprecision mediump int;\n"
    "\tprecision highp float;\n"
    "\t#define varying   out\n"
    "\t#define attribute in\n"
    "\t#define texture2D texture\n",
    "\t#version 300 es\n"
    "\tprecision mediump int;\n"
    "\tprecision highp float;\n"
    "\t#define varying     in\n"
    "\t#define texture2D   texture\n"
    "\tout vec4 fragColor;\n",
    false,
    false,
    false,
};

global_variable const char *s_glesFramebufferFetchFragmentHeader =
    "\t#version 300 es\n"
    "\t#extension GL_EXT_shader_framebuffer_fetch : require\n"
    "\tprecision mediump int;\n"
    "\tprecision highp float;\n"
    "\t#define varying     in\n"
    "\t#define texture2D   texture\n"
    "\tlayout(location = 0) inout vec4 fragColor;\n";
#else
global_variable const struct NativeRendererDialect s_rendererDialect = {
    3,
    3,
    SDL_GL_CONTEXT_PROFILE_CORE,
    "gl",
    "core",
    "140",
    "native-gl",
    "\t#version 140\n"
    "\tprecision lowp  int;\n"
    "\tprecision highp float;\n"
    "\t#define varying   out\n"
    "\t#define attribute in\n"
    "\t#define texture2D texture\n",
    "\t#version 140\n"
    "\tprecision lowp  int;\n"
    "\tprecision highp float;\n"
    "\t#define varying     in\n"
    "\t#define texture2D   texture\n"
    "\tout vec4 fragColor;\n",
    true,
    true,
    true,
};
#endif

global_variable BlendMode s_previousBlendMode = BM_NONE;
global_variable int s_previousDepthMode = 0;
global_variable int s_previousStencilMode = 0;
global_variable int s_previousScissorState = 0;
global_variable int s_previousOffscreenState = 0;
global_variable RECT16 s_previousOffscreen = {0, 0, 0, 0};

global_variable ShaderID s_previousShader = (ShaderID)-1;
global_variable b32 s_psxFramebufferFetchEnabled = false;

global_variable TextureID s_rgLutTexture = (TextureID)-1;
// NOTE(penta3): Single persistent VRAM texture, matching real PS1's single
// 1MB VRAM. PS1 page-flips two windows *inside* one VRAM; it never keeps two
// full copies. The old double buffer existed only to orphan the texture on the
// per-frame full re-upload, which no longer happens (we upload dirty rects).
#define NATIVE_VRAM_DIRTY_RECT_CAP 128
#define NATIVE_VRAM_TILE_SIZE      8
#define NATIVE_VRAM_TILE_COLS      (VRAM_WIDTH / NATIVE_VRAM_TILE_SIZE)
#define NATIVE_VRAM_TILE_ROWS      (VRAM_HEIGHT / NATIVE_VRAM_TILE_SIZE)
#define NATIVE_VRAM_TILE_COUNT     (NATIVE_VRAM_TILE_COLS * NATIVE_VRAM_TILE_ROWS)
#define NATIVE_VRAM_TILE_WORDS     (NATIVE_VRAM_TILE_COUNT / 32)

// NOTE(aalhendi): Native splits PS1 VRAM between a CPU mirror and one packed
// GPU texture. CPU writes are uploaded before GPU reads; GPU-newer tiles are
// resolved into the CPU mirror only when game code reads those VRAM regions.
struct NativeVramState
{
	TextureID texture;
	u16 cpuPixels[VRAM_WIDTH * VRAM_HEIGHT];
	RECT16 cpuDirtyRects[NATIVE_VRAM_DIRTY_RECT_CAP];
	u32 gpuNewerTiles[NATIVE_VRAM_TILE_WORDS];
	s32 cpuDirtyRectCount;
};

global_variable struct NativeVramState s_vram;

struct NativeRenderTarget
{
	TextureID texture;
	GLuint framebuffer;
	GLuint stencilBuffer;
	s32 width;
	s32 height;
};

global_variable struct NativeRenderTarget s_mainRenderTarget;
global_variable struct NativeRenderTarget s_offscreenRenderTarget;
global_variable struct NativeRenderTarget s_presentResolveTarget;
global_variable int s_internalResolutionScale = 1;
global_variable int s_mainLogicalWidth = 1;
global_variable int s_mainLogicalHeight = 1;

global_variable TextureID s_whiteTexture = (TextureID)-1;
global_variable TextureID s_lastBoundTexture = (TextureID)-1;

TextureID NativeRenderer_GetVRAMTexture(void)
{
	return s_vram.texture;
}

TextureID NativeRenderer_GetWhiteTexture(void)
{
	return s_whiteTexture;
}

int g_windowWidth = 0;
int g_windowHeight = 0;

global_variable int s_presentAspectW = 4;
global_variable int s_presentAspectH = 3;
global_variable SDL_Rect s_presentViewport = {0, 0, 0, 0};

int g_dbg_wireframeMode = 0;
int g_dbg_texturelessMode = 0;

int g_cfg_bilinearFiltering = 0;

// NOTE(aalhendi): Pack native RGBA render targets into the persistent RG8 VRAM
// texture on the GPU instead of a GPU-to-CPU-to-GPU round trip.
global_variable GLuint s_packShader = 0;
global_variable GLint s_packFlipYLoc = -1;
global_variable GLuint s_presentVramShader = 0;
global_variable GLint s_presentVramSourceRectLoc = -1;
global_variable GLuint s_vramQuadVAO = 0;
global_variable GLuint s_vramQuadVBO = 0;
global_variable GLuint s_presentFramebuffer = 0;
global_variable GLuint s_presentRenderbuffer = 0;
// Shutdown can be reached after SDL starts but before a GL/GLES context or
// entry-point table exists. Keep that failure path free of unresolved GL calls.
global_variable b32 s_rendererApiReady = false;

internal int NativeRenderer_InitialiseGLContext(char *windowName, int fullscreen);
internal int NativeRenderer_InitialiseGLExt(void);
internal b32 NativeRenderer_HasExtension(const char *name);
internal void NativeRenderer_UpdatePresentSurface(void);
internal void NativeRenderer_BindPresentFramebuffer(void);
internal void NativeRenderer_DestroyTexture(TextureID texture);
internal void NativeRenderer_SetScissorState(int enable);
internal void NativeRenderer_EnableDepth(int enable);
internal void NativeRenderer_SetViewPort(int x, int y, int width, int height);
internal void NativeRenderer_SetPresentationAspect(int width, int height);
internal void NativeRenderer_UpdatePresentationViewport(void);
internal void NativeRenderer_ClearPresentationBars(void);
internal void NativeRenderer_SetWireframe(int enable);
internal void NativeRenderer_InitRenderTarget(struct NativeRenderTarget *target);
internal void NativeRenderer_DestroyRenderTarget(struct NativeRenderTarget *target);
internal void NativeRenderer_EnsureRenderTarget(struct NativeRenderTarget *target, int width, int height);
internal void NativeRenderer_BindMainRenderTarget(void);
internal void NativeRenderer_DrawVRAMRegion(int x, int y, int width, int height);
internal void NativeRenderer_LoadRenderTargetFromVRAM(struct NativeRenderTarget *target, int x, int y, int sourceWidth, int sourceHeight);
#if defined(CTR_NATIVE_GPU_TIMERS)
internal void NativeRenderer_ResolveGpuMeasurements(b32 waitForResults);
#endif

global_variable GLuint s_glVertexArray[2];
global_variable GLuint s_glVertexBuffer[2];
global_variable int s_curVertexBuffer = 0;
global_variable int s_boundVertexBuffer = -1;

global_variable GLuint s_glVramFramebuffer;


internal int NativeRenderer_InitialiseGLContext(char *windowName, int fullscreen)
{
#if defined(SDL_PLATFORM_IOS)
	SDL_WindowFlags windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#else
	SDL_WindowFlags windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#endif
	int minorVersion = s_rendererDialect.contextMinor;

	if (fullscreen)
	{
		windowFlags |= SDL_WINDOW_FULLSCREEN;
	}

	// Cocoa chooses CGL or EGL window setup while creating the window, so the
	// requested profile must be visible before SDL_CreateWindow.
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, s_rendererDialect.contextMajor);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, s_rendererDialect.contextProfile);

	g_window = SDL_CreateWindow(windowName, g_windowWidth, g_windowHeight, windowFlags);

	if (g_window == NULL)
	{
		NATIVE_RENDERER_ERROR("Failed to initialise SDL window: %s\n", SDL_GetError());
		return 0;
	}

	// Desktop GL retains the established 3.x minor fallback. GLES is pinned to
	// 3.0 because vertex arrays and read-framebuffer/pack-row-length behavior
	// are required by the shared renderer and are unavailable in ES 2.
	do
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);

		if (SDL_GL_CreateContext(g_window))
		{
			int pointWidth = 0;
			int pointHeight = 0;
			int pixelWidth = 0;
			int pixelHeight = 0;

			SDL_GetWindowSize(g_window, &pointWidth, &pointHeight);
			if (!SDL_GetWindowSizeInPixels(g_window, &pixelWidth, &pixelHeight))
			{
				pixelWidth = pointWidth;
				pixelHeight = pointHeight;
			}

			if ((pixelWidth > 0) && (pixelHeight > 0))
			{
				g_windowWidth = pixelWidth;
				g_windowHeight = pixelHeight;
			}

			NativeRenderer_UpdatePresentSurface();
			NATIVE_RENDERER_LOG("*Window size: %dx%d points, %dx%d pixels\n", pointWidth, pointHeight, pixelWidth,
			                    pixelHeight);
			NATIVE_RENDERER_LOG("*Presentation objects: framebuffer=%u renderbuffer=%u\n", s_presentFramebuffer,
			                    s_presentRenderbuffer);
			return 1;
		}

		minorVersion--;

	} while (minorVersion >= 0);

	NATIVE_RENDERER_ERROR("Failed to initialise - %s %d.x is not supported: %s\n", s_rendererDialect.apiName,
	                      s_rendererDialect.contextMajor, SDL_GetError());
	return 0;
}

internal void NativeRenderer_UpdatePresentSurface(void)
{
	if (g_window == NULL)
	{
		s_presentFramebuffer = 0;
		s_presentRenderbuffer = 0;
		return;
	}

	const SDL_PropertiesID properties = SDL_GetWindowProperties(g_window);
	s_presentFramebuffer =
	    (GLuint)SDL_GetNumberProperty(properties, SDL_PROP_WINDOW_UIKIT_OPENGL_FRAMEBUFFER_NUMBER, 0);
	s_presentRenderbuffer =
	    (GLuint)SDL_GetNumberProperty(properties, SDL_PROP_WINDOW_UIKIT_OPENGL_RENDERBUFFER_NUMBER, 0);
}

internal void NativeRenderer_BindPresentFramebuffer(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, s_presentFramebuffer);
}

internal b32 NativeRenderer_HasExtension(const char *name)
{
	GLint extensionCount = 0;

	if ((name == NULL) || (name[0] == 0) || (glGetStringi == NULL))
	{
		return false;
	}

	glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
	for (GLint i = 0; i < extensionCount; i++)
	{
		const char *extension = (const char *)glGetStringi(GL_EXTENSIONS, (GLuint)i);
		if ((extension != NULL) && (strcmp(extension, name) == 0))
		{
			return true;
		}
	}

	return false;
}

internal int NativeRenderer_InitialiseGLExt(void)
{
#if defined(CTR_NATIVE_RENDERER_GLES)
	const int status = gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
#else
	const int status = gladLoadGL();
#endif

	if (status == 0)
	{
		return 0;
	}

#if defined(CTR_NATIVE_RENDERER_GLES)
	// The checked-in glad GL loader recognizes the "OpenGL ES " version
	// prefix and loads shared GL/ES 3 entry points. Fail at this boundary if a
	// platform resolver does not supply the specific contract we consume.
	if (!GLAD_GL_VERSION_3_0 || (glBindVertexArray == NULL) || (glGenVertexArrays == NULL) ||
	    (glDeleteVertexArrays == NULL) || (glBindFramebuffer == NULL) || (glBlitFramebuffer == NULL) ||
	    (glGetStringi == NULL) || (glReadPixels == NULL) || (glPixelStorei == NULL))
	{
		NATIVE_RENDERER_ERROR("%s\n", "OpenGL ES 3.0 loader contract is incomplete");
		return 0;
	}
#endif

	const char *rend = (const char *)glGetString(GL_RENDERER);
	const char *vendor = (const char *)glGetString(GL_VENDOR);
	NATIVE_RENDERER_LOG("*Video adapter: %s by %s\n", rend, vendor);

	const char *versionStr = (const char *)glGetString(GL_VERSION);
	NATIVE_RENDERER_LOG("*%s version: %s\n", s_rendererDialect.apiName, versionStr);

	const char *glslVersionStr = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
	NATIVE_RENDERER_LOG("*GLSL version: %s\n", glslVersionStr);

#if defined(CTR_NATIVE_RENDERER_GLES)
	s_psxFramebufferFetchEnabled = NativeRenderer_HasExtension("GL_EXT_shader_framebuffer_fetch");
	NATIVE_RENDERER_LOG("*PSX framebuffer fetch: %s\n", s_psxFramebufferFetchEnabled ? "enabled" : "unavailable; two-pass fallback");
#else
	s_psxFramebufferFetchEnabled = false;
#endif

	return 1;
}

int NativeRenderer_InitialiseRender(char *windowName, int width, int height, int fullscreen)
{
	g_windowWidth = width;
	g_windowHeight = height;
	NativeRenderer_SetPresentationAspect(width, height);

	// Due to debugging in fullscreen
	SDL_SetHint(SDL_HINT_WINDOW_ALLOW_TOPMOST, "0");
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

	if (!NativeRenderer_InitialiseGLContext(windowName, fullscreen))
	{
		NATIVE_RENDERER_ERROR("%s\n", "Failed to Initialise GL Context!");
		return 0;
	}

	if (!NativeRenderer_InitialiseGLExt())
	{
		NATIVE_RENDERER_ERROR("%s\n", "Failed to Intialise GL extensions");
		return 0;
	}

	s_rendererApiReady = true;

	return 1;
}

void NativeRenderer_Shutdown(void)
{
	if (!s_rendererApiReady)
	{
		return;
	}

	s_rendererApiReady = false;
	glDeleteVertexArrays(2, s_glVertexArray);
	glDeleteBuffers(2, s_glVertexBuffer);

	NativeRenderer_DestroyRenderTarget(&s_mainRenderTarget);
	NativeRenderer_DestroyRenderTarget(&s_offscreenRenderTarget);
	NativeRenderer_DestroyRenderTarget(&s_presentResolveTarget);
	glDeleteFramebuffers(1, &s_glVramFramebuffer);

	NativeRenderer_DestroyTexture(s_vram.texture);

	NativeRenderer_DestroyTexture(s_whiteTexture);
	NativeRenderer_DestroyTexture(s_rgLutTexture);
	glDeleteProgram(s_packShader);
	glDeleteProgram(s_presentVramShader);
	glDeleteVertexArrays(1, &s_vramQuadVAO);
	glDeleteBuffers(1, &s_vramQuadVBO);
	s_presentFramebuffer = 0;
	s_presentRenderbuffer = 0;
	s_psxFramebufferFetchEnabled = false;
}

#if defined(CTR_NATIVE_GPU_TIMERS)
internal void NativeRenderer_ResolveGpuMeasurements(b32 waitForResults)
{
	for (s32 i = 0; i < NATIVE_GPU_TIMER_QUERY_COUNT; i++)
	{
		struct NativeGpuTimerQuery *query = &s_gpuTimerQueries[i];
		if (!query->pending)
		{
			continue;
		}

		GLint available = 0;
		if (!waitForResults)
		{
			glGetQueryObjectiv(query->id, GL_QUERY_RESULT_AVAILABLE, &available);
			if (!available)
			{
				continue;
			}
		}

		GLuint elapsedNanoseconds;
		glGetQueryObjectuiv(query->id, GL_QUERY_RESULT, &elapsedNanoseconds);
		NativePerf_RecordGpuFrame(query->frameIndex, (f64)elapsedNanoseconds / 1000000.0);
		query->pending = false;
	}
}
#endif

void NativeRenderer_UpdateSwapIntervalState(int swapInterval)
{
	SDL_GL_SetSwapInterval(swapInterval);
}

int NativeRenderer_SetInternalResolutionScale(int scale)
{
	GLint maximumTextureSize = 0;
	int maximumScale = 4;

	if (scale < 1)
	{
		scale = 1;
	}
	if (scale > 4)
	{
		scale = 4;
	}

	if (s_rendererApiReady)
	{
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTextureSize);
		while ((maximumScale > 1) &&
		       (((s_mainLogicalWidth * maximumScale) > maximumTextureSize) ||
		        ((s_mainLogicalHeight * maximumScale) > maximumTextureSize)))
		{
			maximumScale--;
		}
	}

	if (scale > maximumScale)
	{
		scale = maximumScale;
	}
	s_internalResolutionScale = scale;
	return s_internalResolutionScale;
}

void NativeRenderer_BeginScene(void)
{
#if defined(CTR_NATIVE_GPU_TIMERS)
	NativeRenderer_ResolveGpuMeasurements(false);
	const u32 gpuFrameIndex = s_gpuTimerFrameIndex++;
	if (s_gpuTimerSupported && NativePerf_IsEnabled())
	{
		struct NativeGpuTimerQuery *query = &s_gpuTimerQueries[s_gpuTimerNextQuery];
		if (!query->pending)
		{
			query->frameIndex = gpuFrameIndex;
			query->pending = true;
			glBeginQuery(GL_TIME_ELAPSED, query->id);
			s_gpuTimerActive = true;
			s_gpuTimerNextQuery = (s_gpuTimerNextQuery + 1) % NATIVE_GPU_TIMER_QUERY_COUNT;
		}
	}
#endif

	NativePerf_BeginScope(NATIVE_PERF_BUCKET_RENDERER_BEGIN_SCENE);
	s_lastBoundTexture = 0;

	NativeRenderer_UpdatePresentationViewport();
	NativeRenderer_ClearPresentationBars();
	NativeRenderer_BindMainRenderTarget();

	NativeRenderer_UpdateVRAM();
	if (!activeDrawEnv.isbg)
	{
		NativeRenderer_LoadRenderTargetFromVRAM(&s_mainRenderTarget, activeDispEnv.disp.x, activeDispEnv.disp.y,
		                                        s_mainLogicalWidth, s_mainLogicalHeight);
	}
	else
	{
		glClear(GL_STENCIL_BUFFER_BIT);
	}
	NativeRenderer_SetViewPort(0, 0, s_mainRenderTarget.width, s_mainRenderTarget.height);

	if (g_dbg_wireframeMode)
	{
		NativeRenderer_SetWireframe(1);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	NativePerf_EndScope(NATIVE_PERF_BUCKET_RENDERER_BEGIN_SCENE);
}

void NativeRenderer_EndGpuFrame(void)
{
#if defined(CTR_NATIVE_GPU_TIMERS)
	if (s_gpuTimerActive)
	{
		glEndQuery(GL_TIME_ELAPSED);
		s_gpuTimerActive = false;
	}
#endif
}

void NativeRenderer_FinishGpuMeasurements(void)
{
#if defined(CTR_NATIVE_GPU_TIMERS)
	NativeRenderer_EndGpuFrame();
	if (!s_gpuTimerSupported)
	{
		return;
	}

	NativeRenderer_ResolveGpuMeasurements(true);
	for (s32 i = 0; i < NATIVE_GPU_TIMER_QUERY_COUNT; i++)
	{
		glDeleteQueries(1, &s_gpuTimerQueries[i].id);
	}
	SDL_memset(s_gpuTimerQueries, 0, sizeof(s_gpuTimerQueries));
	s_gpuTimerSupported = false;
#endif
}

void NativeRenderer_EndScene(void)
{
	if (s_previousOffscreenState)
	{
		NativeRenderer_SetOffscreenState(&s_previousOffscreen, 0);
	}

	if (g_dbg_wireframeMode)
	{
		NativeRenderer_SetWireframe(0);
	}

	glBindVertexArray(0);
}

//----------------------------------------------------------------------------------------

global_variable u8 rgLUT[LUT_WIDTH * LUT_HEIGHT * sizeof(u32)];

internal int NativeRenderer_IntAbs(int value)
{
	return value < 0 ? -value : value;
}

internal int NativeRenderer_GCD(int a, int b)
{
	a = NativeRenderer_IntAbs(a);
	b = NativeRenderer_IntAbs(b);

	while (b != 0)
	{
		int t = a % b;
		a = b;
		b = t;
	}

	return a;
}

internal void NativeRenderer_SetPresentationAspect(int width, int height)
{
	const int divisor = NativeRenderer_GCD(width, height);

	if ((width <= 0) || (height <= 0) || (divisor <= 0))
	{
		return;
	}

	s_presentAspectW = width / divisor;
	s_presentAspectH = height / divisor;
}

internal void NativeRenderer_UpdatePresentationViewport(void)
{
	int viewportW;
	int viewportH;

	if ((g_windowWidth <= 0) || (g_windowHeight <= 0) || (s_presentAspectW <= 0) || (s_presentAspectH <= 0))
	{
		s_presentViewport.x = 0;
		s_presentViewport.y = 0;
		s_presentViewport.w = g_windowWidth;
		s_presentViewport.h = g_windowHeight;
		return;
	}

	viewportW = g_windowWidth;
	viewportH = (viewportW * s_presentAspectH) / s_presentAspectW;

	if (viewportH > g_windowHeight)
	{
		viewportH = g_windowHeight;
		viewportW = (viewportH * s_presentAspectW) / s_presentAspectH;
	}

	if (viewportW < 1)
	{
		viewportW = 1;
	}
	if (viewportH < 1)
	{
		viewportH = 1;
	}

	s_presentViewport.w = viewportW;
	s_presentViewport.h = viewportH;
	s_presentViewport.x = (g_windowWidth - viewportW) / 2;
	s_presentViewport.y = (g_windowHeight - viewportH) / 2;
}

internal void NativeRenderer_InitRenderTarget(struct NativeRenderTarget *target)
{
	target->texture = (TextureID)-1;
	glGenTextures(1, &target->texture);
	glBindTexture(GL_TEXTURE_2D, target->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenRenderbuffers(1, &target->stencilBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, target->stencilBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, 1, 1);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glGenFramebuffers(1, &target->framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, target->framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target->texture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, target->stencilBuffer);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		NATIVE_RENDERER_ERROR("%s\n", "failed to create RGBA/stencil render target");
	}
	NativeRenderer_BindPresentFramebuffer();
}

internal void NativeRenderer_DestroyRenderTarget(struct NativeRenderTarget *target)
{
	glDeleteFramebuffers(1, &target->framebuffer);
	glDeleteRenderbuffers(1, &target->stencilBuffer);
	NativeRenderer_DestroyTexture(target->texture);
	SDL_memset(target, 0, sizeof(*target));
	target->texture = (TextureID)-1;
}

internal void NativeRenderer_EnsureRenderTarget(struct NativeRenderTarget *target, int width, int height)
{
	if (width < 1)
	{
		width = 1;
	}
	if (height < 1)
	{
		height = 1;
	}

	if ((target->width == width) && (target->height == height))
	{
		return;
	}

	glBindTexture(GL_TEXTURE_2D, target->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, target->stencilBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	target->width = width;
	target->height = height;
	s_lastBoundTexture = (TextureID)-1;
}

internal void NativeRenderer_BindMainRenderTarget(void)
{
	int width = activeDispEnv.disp.w;
	int height = activeDispEnv.disp.h;
	if ((width <= 0) || (height <= 0))
	{
		width = activeDrawEnv.clip.w;
		height = activeDrawEnv.clip.h;
	}

	s_mainLogicalWidth = width > 0 ? width : 1;
	s_mainLogicalHeight = height > 0 ? height : 1;
	NativeRenderer_EnsureRenderTarget(&s_mainRenderTarget, s_mainLogicalWidth * s_internalResolutionScale,
	                                  s_mainLogicalHeight * s_internalResolutionScale);
	glBindFramebuffer(GL_FRAMEBUFFER, s_mainRenderTarget.framebuffer);
}

internal void NativeRenderer_DrawVRAMRegion(int x, int y, int width, int height)
{
	glUseProgram(s_presentVramShader);
	glUniform4f(s_presentVramSourceRectLoc, (float)x, (float)y, (float)width, (float)height);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, s_vram.texture);
	glBindVertexArray(s_vramQuadVAO);
	NativeRenderer_DrawTriangles(0, 2);
}

internal void NativeRenderer_LoadRenderTargetFromVRAM(struct NativeRenderTarget *target, int x, int y, int sourceWidth, int sourceHeight)
{
	NativePerf_BeginScope(NATIVE_PERF_BUCKET_RENDERER_RESTORE_VRAM);
	const ShaderID previousShader = s_previousShader;
	const TextureID previousTexture = s_lastBoundTexture;
	const BlendMode previousBlendMode = s_previousBlendMode;
	const int previousScissorState = s_previousScissorState;

	NativeRenderer_UpdateVRAM();
	glBindFramebuffer(GL_FRAMEBUFFER, target->framebuffer);
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);
	glViewport(0, 0, target->width, target->height);
	NativeRenderer_DrawVRAMRegion(x, y, sourceWidth, sourceHeight);
	glClear(GL_STENCIL_BUFFER_BIT);
	glEnable(GL_STENCIL_TEST);

	if (s_boundVertexBuffer >= 0)
	{
		glBindVertexArray(s_glVertexArray[s_boundVertexBuffer]);
	}
	else
	{
		glBindVertexArray(0);
	}
	glUseProgram(previousShader == (ShaderID)-1 ? 0 : previousShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, previousTexture == (TextureID)-1 ? 0 : previousTexture);
	s_previousShader = previousShader;
	s_lastBoundTexture = previousTexture;
	s_previousBlendMode = BM_NONE;
	s_previousScissorState = 0;
	NativeRenderer_SetBlendMode(previousBlendMode);
	NativeRenderer_SetScissorState(previousScissorState);
	NativePerf_EndScope(NATIVE_PERF_BUCKET_RENDERER_RESTORE_VRAM);
}

internal void NativeRenderer_ClearHostRect(int x, int y, int width, int height)
{
	if ((width <= 0) || (height <= 0))
	{
		return;
	}

	glScissor(x, y, width, height);
	glClear(GL_COLOR_BUFFER_BIT);
}

internal void NativeRenderer_ClearPresentationBars(void)
{
	GLint previousScissorBox[4];
	GLfloat previousClearColor[4];
	const GLboolean previousScissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
	const int viewportRight = s_presentViewport.x + s_presentViewport.w;
	const int viewportTop = s_presentViewport.y + s_presentViewport.h;

	if ((g_windowWidth <= 0) || (g_windowHeight <= 0))
	{
		return;
	}

	if ((s_presentViewport.x == 0) && (s_presentViewport.y == 0) && (s_presentViewport.w == g_windowWidth) && (s_presentViewport.h == g_windowHeight))
	{
		return;
	}

	glGetIntegerv(GL_SCISSOR_BOX, previousScissorBox);
	glGetFloatv(GL_COLOR_CLEAR_VALUE, previousClearColor);

	glEnable(GL_SCISSOR_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	NativeRenderer_ClearHostRect(0, 0, s_presentViewport.x, g_windowHeight);
	NativeRenderer_ClearHostRect(viewportRight, 0, g_windowWidth - viewportRight, g_windowHeight);
	NativeRenderer_ClearHostRect(s_presentViewport.x, 0, s_presentViewport.w, s_presentViewport.y);
	NativeRenderer_ClearHostRect(s_presentViewport.x, viewportTop, s_presentViewport.w, g_windowHeight - viewportTop);

	if (previousScissorEnabled)
	{
		glEnable(GL_SCISSOR_TEST);
		glScissor(previousScissorBox[0], previousScissorBox[1], previousScissorBox[2], previousScissorBox[3]);
	}
	else
	{
		glDisable(GL_SCISSOR_TEST);
	}

	glClearColor(previousClearColor[0], previousClearColor[1], previousClearColor[2], previousClearColor[3]);
	s_previousScissorState = previousScissorEnabled ? 1 : 0;
}

void NativeRenderer_ResetDevice(void)
{
	NativeRenderer_UpdatePresentSurface();
	NativeRenderer_UpdatePresentationViewport();
	NativeRenderer_UpdateSwapIntervalState(0);
}

typedef struct
{
	// shader itself
	ShaderID shader;

	GLint projectionLoc;
	GLint bilinearFilterLoc;
	GLint texelSizeLoc;
	GLint texLoc;
	GLint lutLoc;
	GLint psxSemiTransPassLoc;
	GLint psxFramebufferFetchBlendModeLoc;
	GLint psxDrawMaskSetLoc;
	GLint psxTextureOutputStpLoc;
} GTEShader;

internal int NativeRenderer_Shader_CheckShaderStatus(GLuint shader);
internal int NativeRenderer_Shader_CheckProgramStatus(GLuint program);
internal ShaderID NativeRenderer_Shader_Compile(const char *source, bool isPsxShader);
internal void NativeRenderer_GenerateCommonTextures(void);
internal void NativeRenderer_CompilePSXShader(GTEShader *sh, const char *source);
internal void NativeRenderer_InitialisePSXShaders(void);
internal void NativeRenderer_InitRG8LUT(void);
internal void NativeRenderer_Ortho2D(float left, float right, float bottom, float top, float znear, float zfar);
internal void NativeRenderer_SetShader(const ShaderID shader);
internal void NativeRenderer_SyncGpuVRAMToCPU(int x, int y, int w, int h);
internal void NativeRenderer_ResolveVRAMRead(int x, int y, int w, int h);
internal void NativeRenderer_GpuPackTextureToVRAM(TextureID sourceTexture, int x, int y, int w, int h, b32 flipY);

global_variable GTEShader s_gteShader4;
global_variable GTEShader s_gteShader8;
global_variable GTEShader s_gteShader16;
global_variable GTEShader s_gteShader32Rgba;

GLint u_projectionLoc;
GLint u_bilinearFilterLoc;
GLint u_texelSizeLoc;
GLint u_psxSemiTransPassLoc;
GLint u_psxFramebufferFetchBlendModeLoc;
GLint u_psxDrawMaskSetLoc;
GLint u_psxTextureOutputStpLoc;

#define GPU_SAMPLE_TEXTURE_4BIT_FUNC                                             \
	"	// returns 16 bit colour\n"                                                \
	"	vec2 samplePSX(vec2 tc) {\n"                                               \
	"		vec2 uv = (tc * vec2(0.25, 1.0) + v_page_clut.xy) * c_VRAMTexel;\n"       \
	"		vec2 comp = VRAM(uv);\n"                                                  \
	"		int index = int(fract(tc.x / 4.0 + 0.0001) * 4.0);\n"                     \
	"		float v = _idx2(comp, index / 2) * (255.0 / 16.0);\n"                     \
	"		float f = floor(v + 0.001);\n"                                            \
	"		vec2 c = vec2((v - f) * 16.0, f);\n"                                      \
	"		vec2 clut_pos = v_page_clut.zw;\n"                                        \
	"		clut_pos.x += mix(c[0], c[1], mod(float(index), 2.0)) * c_VRAMTexel.x;\n" \
	"		return VRAM(clut_pos);\n"                                                 \
	"	}\n"

#define GPU_SAMPLE_TEXTURE_8BIT_FUNC                                      \
	"	// returns 16 bit colour\n"                                         \
	"	vec2 samplePSX(vec2 tc) {\n"                                        \
	"		vec2 uv = (tc * vec2(0.5, 1.0) + v_page_clut.xy) * c_VRAMTexel;\n" \
	"		vec2 comp = VRAM(uv);\n"                                           \
	"		vec2 clut_pos = v_page_clut.zw;\n"                                 \
	"		int index = int(mod(tc.x, 2.0));\n"                                \
	"		clut_pos.x += _idx2(comp, index) * 255.0 * c_VRAMTexel.x;\n"       \
	"		return VRAM(clut_pos);\n"                                          \
	"	}\n"

#define GPU_SAMPLE_TEXTURE_16BIT_FUNC                    \
	"	vec2 samplePSX(vec2 tc) {\n"                       \
	"		vec2 uv = (tc + v_page_clut.xy) * c_VRAMTexel;\n" \
	"		return VRAM(uv);\n"                               \
	"	}\n"

#define GPU_FETCH_VRAM_FUNC                                        \
	"	const vec2 c_VRAMTexel = vec2(1.0 / 1024.0, 1.0 / 512.0);\n" \
	"	uniform sampler2D s_texture;\n"                              \
	"	vec2 VRAM(vec2 uv) { return texture2D(s_texture, uv).rg; }\n"

#define GPU_DITHERING                                             \
	"	const mat4 c_dither = mat4(\n"                              \
	"		-4.0,  +0.0,  -3.0,  +1.0,\n"                              \
	"		+2.0,  -2.0,  +3.0,  -1.0,\n"                              \
	"		-3.0,  +1.0,  -4.0,  +0.0,\n"                              \
	"		+3.0,  -1.0,  +2.0,  -2.0) / 255.0;\n"                     \
	"	vec4 dither(vec4 color) {\n"                                \
	"		ivec2 dc = ivec2(mod(floor(v_ditherCoord), 4.0));\n"       \
	"		color.xyz += vec3(c_dither[dc.x][dc.y] * v_texcoord.w);\n" \
	"		return color;\n"                                           \
	"	}\n"

#define GPU_ARRAY_FUNC "	float _idx2(vec2 array, int idx) { return array[idx]; }\n"

#define GPU_FRAMEBUFFER_FETCH_BLEND_FUNC                                                                     \
	"\t#ifdef PSX_FRAMEBUFFER_FETCH\n"                                                                         \
	"\tuniform int psxFramebufferFetchBlendMode;\n"                                                 \
	"\tvec4 applyPSXFramebufferFetch(vec4 source) {\n"                                               \
	"\t\tif(psxSemiTransPass != 3 || sampledStp < 0.5) { return source; }\n"                         \
	"\t\tvec3 destination = (sampledNonStp >= 0.5) ? source.rgb : fragColor.rgb;\n"                \
	"\t\tvec3 blended = source.rgb;\n"                                                              \
	"\t\tif(psxFramebufferFetchBlendMode == 1) { blended = source.rgb * 0.5 + destination * 0.5; }\n" \
	"\t\telse if(psxFramebufferFetchBlendMode == 2) { blended = source.rgb + destination; }\n"      \
	"\t\telse if(psxFramebufferFetchBlendMode == 3) { blended = destination - source.rgb; }\n"      \
	"\t\telse if(psxFramebufferFetchBlendMode == 4) { blended = source.rgb * 0.25 + destination; }\n" \
	"\t\treturn vec4(blended, source.a);\n"                                                      \
	"\t}\n"                                                                                            \
	"\t#else\n"                                                                                       \
	"\tvec4 applyPSXFramebufferFetch(vec4 source) { return source; }\n"                             \
	"\t#endif\n"

#define GPU_STP_PASS_FUNC                                                                     \
	"	float texelVisible(vec2 rg) { return float(rg.x + rg.y > 0.0); }\n"                     \
	"	float stpWeight(vec2 rg) { return step(0.5, rg.y); }\n"                                 \
	"	bool discardForSemiTransPass(float visible, float stpVisible, float nonStpVisible) {\n" \
	"		if(visible < 0.5) { return true; }\n"                                                  \
	"		if(psxSemiTransPass == 1 && nonStpVisible < 0.5) { return true; }\n"                   \
	"		if(psxSemiTransPass == 2 && stpVisible < 0.5) { return true; }\n"                      \
	"		if(psxSemiTransPass == 3 && stpVisible < 0.5 && nonStpVisible < 0.5) { return true; }\n" \
	"		return false;\n"                                                                       \
	"	}\n" GPU_FRAMEBUFFER_FETCH_BLEND_FUNC

#define GPU_FRAGMENT_SAMPLE_SHADER(bit)                                                                                                               \
	GPU_FETCH_VRAM_FUNC                                                                                                                               \
	GPU_ARRAY_FUNC                                                                                                                                    \
	GPU_SAMPLE_TEXTURE_##bit##BIT_FUNC                                                                                                                \
	    "	uniform sampler2D s_rgLut;\n"                                                                                                               \
	    "	uniform int bilinearFilter;\n"                                                                                                              \
	    "	uniform int psxSemiTransPass;\n"                                                                                                            \
	    "	uniform int psxDrawMaskSet;\n"                                                                                                              \
	    "	uniform int psxTextureOutputStp;\n"                                                                                                         \
	    "	float sampledStp = 0.0;\n"                                                                                                                  \
	    "	float sampledNonStp = 0.0;\n"                                                                                                               \
	    "	const vec2 c_LUTTexel = vec2(1.0 / 256.0, 1.0 / 256.0);\n"                                                                                  \
	    "	vec4 lut(vec2 rg) { return texture2D(s_rgLut, rg - c_LUTTexel * 0.0001); }\n" GPU_STP_PASS_FUNC "	vec4 bilinearTextureSample(vec2 P) {\n" \
	    "		vec2 frac = fract(P);\n"                                                                                                                   \
	    "		vec2 pixel = floor(P);\n"                                                                                                                  \
	    "		vec2 C11 = samplePSX(pixel);\n"                                                                                                            \
	    "		vec2 C21 = samplePSX(pixel + vec2(1.0, 0.0));\n"                                                                                           \
	    "		vec2 C12 = samplePSX(pixel + vec2(0.0, 1.0));\n"                                                                                           \
	    "		vec2 C22 = samplePSX(pixel + vec2(1.0, 1.0));\n"                                                                                           \
	    "		float v11 = texelVisible(C11);\n"                                                                                                          \
	    "		float v21 = texelVisible(C21);\n"                                                                                                          \
	    "		float v12 = texelVisible(C12);\n"                                                                                                          \
	    "		float v22 = texelVisible(C22);\n"                                                                                                          \
	    "		float s11 = v11 * stpWeight(C11);\n"                                                                                                       \
	    "		float s21 = v21 * stpWeight(C21);\n"                                                                                                       \
	    "		float s12 = v12 * stpWeight(C12);\n"                                                                                                       \
	    "		float s22 = v22 * stpWeight(C22);\n"                                                                                                       \
	    "		float n11 = v11 - s11;\n"                                                                                                                  \
	    "		float n21 = v21 - s21;\n"                                                                                                                  \
	    "		float n12 = v12 - s12;\n"                                                                                                                  \
	    "		float n22 = v22 - s22;\n"                                                                                                                  \
	    "		float ax1 = mix(v11, v21, frac.x);\n"                                                                                                      \
	    "		float ax2 = mix(v12, v22, frac.x);\n"                                                                                                      \
	    "		float axm = mix(ax1, ax2, frac.y);\n"                                                                                                      \
	    "		float sx1 = mix(s11, s21, frac.x);\n"                                                                                                      \
	    "		float sx2 = mix(s12, s22, frac.x);\n"                                                                                                      \
	    "		float stp = mix(sx1, sx2, frac.y);\n"                                                                                                      \
	    "		float nx1 = mix(n11, n21, frac.x);\n"                                                                                                      \
	    "		float nx2 = mix(n12, n22, frac.x);\n"                                                                                                      \
	    "		float nonStp = mix(nx1, nx2, frac.y);\n"                                                                                                   \
	    "		vec2 rg = mix(mix(C11, C21, frac.x), mix(C12, C22, frac.x), frac.y);\n"                                                                    \
	    "		sampledStp = stp;\n"                                                                                                                       \
	    "		sampledNonStp = nonStp;\n"                                                                                                                 \
	    "		if(discardForSemiTransPass(axm, stp, nonStp)) { discard; }\n"                                                                              \
	    "		vec4 x1 = mix(lut(C11), lut(C21), frac.x);\n"                                                                                              \
	    "		vec4 x2 = mix(lut(C12), lut(C22), frac.x);\n"                                                                                              \
	    "		vec4 t = mix(x1, x2, frac.y);\n"                                                                                                           \
	    "		t.w = 1.0 - t.w;\n"                                                                                                                        \
	    "		return t;\n"                                                                                                                               \
	    "	}\n"                                                                                                                                        \
	    "	vec4 nearestTextureSample(vec2 P) {\n"                                                                                                      \
	    "		vec2 rg = samplePSX(P);\n"                                                                                                                 \
	    "		float visible = texelVisible(rg);\n"                                                                                                       \
	    "		sampledStp = visible * stpWeight(rg);\n"                                                                                                   \
	    "		sampledNonStp = visible - sampledStp;\n"                                                                                                    \
	    "		if(discardForSemiTransPass(visible, sampledStp, visible - sampledStp)) { discard; }\n"                                                     \
	    "		vec4 t = lut(rg);\n"                                                                                                                       \
	    "		t.w = 1.0 - t.w;\n"                                                                                                                        \
	    "		return t;\n"                                                                                                                               \
	    "	}\n"                                                                                                                                        \
	    "	void main() {\n"                                                                                                                            \
	    "		vec4 color = (bilinearFilter > 0) ? bilinearTextureSample(v_texcoord.xy) : nearestTextureSample(v_texcoord.xy);\n"                         \
	    "		vec4 sourceColor = dither(color * v_color);\n"                                                                                              \
	    "		sourceColor.a = (psxDrawMaskSet != 0 || (psxTextureOutputStp != 0 && sampledStp >= 0.5)) ? 1.0 : 0.0;\n"                                   \
	    "		fragColor = applyPSXFramebufferFetch(sourceColor);\n"                                                                                      \
	    "	}\n"

global_variable const char *gpu_shader_common = "	varying vec4 v_texcoord;\n"
                                                "	varying vec4 v_color;\n"
                                                "	varying vec4 v_page_clut;\n"
                                                "	varying vec2 v_ditherCoord;\n"
                                                "	varying float v_z;\n";

const char *gte_shader_4 = GPU_FRAGMENT_SAMPLE_SHADER(4);
const char *gte_shader_8 = GPU_FRAGMENT_SAMPLE_SHADER(8);
const char *gte_shader_16 = GPU_FRAGMENT_SAMPLE_SHADER(16);
const char *gte_shader_32_rgba = "	uniform sampler2D s_texture;\n"
                                 "	uniform int psxDrawMaskSet;\n"
                                 "	uniform vec2 texelSize;\n"
                                 "	void main() {\n"
                                 "		vec2 tc = v_texcoord.xy * texelSize + texelSize * 0.5;\n"
                                 "		vec4 color = texture2D(s_texture, tc);\n"
                                 "		fragColor = dither(color * v_color);\n"
                                 "		fragColor.a = float(psxDrawMaskSet);\n"
                                 "	}\n";

#define GTE_PERSPECTIVE_CORRECTION "	gl_Position = Projection * vec4(a_position.xy, 0.0, 1.0);\n"

#define GTE_VERTEX_SHADER                                                                                          \
	"	attribute vec4 a_position;\n"                                                                                \
	"	attribute vec4 a_texcoord; // uv, color multiplier, dither\n"                                                \
	"	attribute vec4 a_color;\n"                                                                                   \
	"	attribute vec4 a_extra; // texcoord.xy ofs, unused.xy\n"                                                     \
	"	uniform mat4 Projection;\n"                                                                                  \
	"	const vec2 c_UVFudge = vec2(0.00025, 0.00025);\n"                                                            \
	"	void main() {\n"                                                                                             \
	"		v_ditherCoord = a_position.xy;\n"                                                                           \
	"		v_texcoord = a_texcoord;\n"                                                                                 \
	"		v_texcoord.xy += a_extra.xy * 0.5;\n"                                                                       \
	"		v_color = a_color;\n"                                                                                       \
	"		v_color.xyz *= a_texcoord.z;\n"                                                                             \
	"		v_page_clut.x = fract(a_position.z / 16.0) * 1024.0;\n"                                                     \
	"		v_page_clut.y = floor(a_position.z / 16.0) * 256.0;\n"                                                      \
	"		v_page_clut.z = fract(a_position.w / 64.0);\n"                                                              \
	"		v_page_clut.w = floor(a_position.w / 64.0) / 512.0;\n"                                                      \
	"		v_page_clut.xy += c_UVFudge;\n"                                                                             \
	"		v_page_clut.zw += c_UVFudge;\n" GTE_PERSPECTIVE_CORRECTION "		v_z = (gl_Position.z - 40.0) * 0.005;\n" \
	"	}\n"

internal int NativeRenderer_Shader_CheckShaderStatus(GLuint shader)
{
	char info[1024];
	GLint result;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);

	if (result == GL_TRUE)
	{
		return 1;
	}

	glGetShaderInfoLog(shader, sizeof(info), NULL, info);
	if (info[0] && strlen(info) > 8)
	{
		NATIVE_RENDERER_ERROR("%s\n", info);
		assert(0);
	}

	return 0;
}

internal int NativeRenderer_Shader_CheckProgramStatus(GLuint program)
{
	char info[1024];
	GLint result;

	glGetProgramiv(program, GL_LINK_STATUS, &result);

	if (result == GL_TRUE)
	{
		return 1;
	}

	glGetProgramInfoLog(program, sizeof(info), NULL, info);
	if (info[0] && strlen(info) > 8)
	{
		NATIVE_RENDERER_ERROR("%s\n", info);
		assert(0);
	}

	return 0;
}

internal ShaderID NativeRenderer_Shader_Compile(const char *source, bool isPsxShader)
{
	const char *GLSL_HEADER_VERT = s_rendererDialect.vertexShaderHeader;
	const char *GLSL_HEADER_FRAG = s_rendererDialect.fragmentShaderHeader;

	char extra_vs_defines[1024];
	char extra_fs_defines[1024];
	extra_vs_defines[0] = 0;
	extra_fs_defines[0] = 0;

	strcat(extra_vs_defines, "#define VERTEX\n");
	strcat(extra_fs_defines, "#define FRAGMENT\n");
#if defined(CTR_NATIVE_RENDERER_GLES)
	if (isPsxShader && s_psxFramebufferFetchEnabled)
	{
		GLSL_HEADER_FRAG = s_glesFramebufferFetchFragmentHeader;
		strcat(extra_fs_defines, "#define PSX_FRAMEBUFFER_FETCH\n");
	}
#endif
	if (g_cfg_bilinearFiltering)
	{
		strcat(extra_fs_defines, "#define BILINEAR_FILTER\n");
	}

	const char *vs_list_psx[] = {GLSL_HEADER_VERT, extra_vs_defines, gpu_shader_common, GTE_VERTEX_SHADER};
	const char *fs_list_psx[] = {GLSL_HEADER_FRAG, extra_fs_defines, gpu_shader_common, GPU_DITHERING, source};
	const char *vs_list_src[] = {
	    GLSL_HEADER_VERT,
	    extra_vs_defines,
	    source,
	};
	const char *fs_list_src[] = {GLSL_HEADER_FRAG, extra_fs_defines, source};

	const char **vs_list = isPsxShader ? vs_list_psx : vs_list_src;
	const char **fs_list = isPsxShader ? fs_list_psx : fs_list_src;
	const int vs_list_cnt = isPsxShader ? 4 : 3;
	const int fs_list_cnt = isPsxShader ? 5 : 3;

	GLuint program = glCreateProgram();

	{
		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, vs_list_cnt, vs_list, NULL);
		glCompileShader(vertexShader);

		if (NativeRenderer_Shader_CheckShaderStatus(vertexShader) == 0)
		{
			NATIVE_RENDERER_ERROR("%s\n", "Failed to compile Vertex Shader!");
		}

		glAttachShader(program, vertexShader);
		glDeleteShader(vertexShader);
	}

	{
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, fs_list_cnt, fs_list, NULL);
		glCompileShader(fragmentShader);

		if (NativeRenderer_Shader_CheckShaderStatus(fragmentShader) == 0)
		{
			NATIVE_RENDERER_ERROR("%s\n", "Failed to compile Fragment Shader!");
		}

		glAttachShader(program, fragmentShader);
		glDeleteShader(fragmentShader);
	}

	glBindAttribLocation(program, a_position, "a_position");
	glBindAttribLocation(program, a_texcoord, "a_texcoord");
	glBindAttribLocation(program, a_color, "a_color");
	glBindAttribLocation(program, a_extra, "a_extra");

	glLinkProgram(program);
	if (NativeRenderer_Shader_CheckProgramStatus(program) == 0)
	{
		NATIVE_RENDERER_ERROR("%s\n", "Failed to link Shader!");
	}

	GLint textureSampler = 0;
	GLint lutSampler = 1;
	glUseProgram(program);
	glUniform1iv(glGetUniformLocation(program, "s_texture"), 1, &textureSampler);
	glUniform1iv(glGetUniformLocation(program, "s_rgLut"), 1, &lutSampler);
	glUseProgram(0);

	return program;
}

//--------------------------------------------------------------------------------------------

internal void NativeRenderer_GenerateCommonTextures(void)
{
	u32 whitePixelData = 0xFFFFFFFF;

	glGenTextures(1, &s_whiteTexture);
	{
		glBindTexture(GL_TEXTURE_2D, s_whiteTexture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &whitePixelData);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glGenTextures(1, &s_rgLutTexture);
	{
		// Texture unit 1 is reserved for the immutable PSX color lookup table.
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, s_rgLutTexture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LUT_WIDTH, LUT_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, &rgLUT);

		glActiveTexture(GL_TEXTURE0);
	}
}

internal void NativeRenderer_CompilePSXShader(GTEShader *sh, const char *source)
{
	sh->shader = NativeRenderer_Shader_Compile(source, true);

	sh->bilinearFilterLoc = glGetUniformLocation(sh->shader, "bilinearFilter");
	sh->projectionLoc = glGetUniformLocation(sh->shader, "Projection");
	sh->texelSizeLoc = glGetUniformLocation(sh->shader, "texelSize");
	sh->texLoc = glGetUniformLocation(sh->shader, "s_texture");
	sh->lutLoc = glGetUniformLocation(sh->shader, "s_rgLut");
	sh->psxSemiTransPassLoc = glGetUniformLocation(sh->shader, "psxSemiTransPass");
	sh->psxFramebufferFetchBlendModeLoc = glGetUniformLocation(sh->shader, "psxFramebufferFetchBlendMode");
	sh->psxDrawMaskSetLoc = glGetUniformLocation(sh->shader, "psxDrawMaskSet");
	sh->psxTextureOutputStpLoc = glGetUniformLocation(sh->shader, "psxTextureOutputStp");
}

internal void NativeRenderer_InitialisePSXShaders(void)
{
	NATIVE_RENDERER_LOG("%s", "*Compiling 4-bit PSX shader\n");
	NativeRenderer_CompilePSXShader(&s_gteShader4, gte_shader_4);
	NATIVE_RENDERER_LOG("%s", "*Compiling 8-bit PSX shader\n");
	NativeRenderer_CompilePSXShader(&s_gteShader8, gte_shader_8);
	NATIVE_RENDERER_LOG("%s", "*Compiling 16-bit PSX shader\n");
	NativeRenderer_CompilePSXShader(&s_gteShader16, gte_shader_16);
	NATIVE_RENDERER_LOG("%s", "*Compiling RGBA PSX shader\n");
	NativeRenderer_CompilePSXShader(&s_gteShader32Rgba, gte_shader_32_rgba);
	NATIVE_RENDERER_LOG("%s", "*PSX shaders ready\n");
}

// NOTE(aalhendi): GPU VRAM pack. Samples an RGBA render texture and writes PS1
// 5:5:5:1 pixels into RG8 VRAM, low byte in R and high byte in G. NEAREST
// sampling and integer channel shifts preserve the packed PS1 pixel value.
global_variable const char *ctr_pack_shader = "#ifdef VERTEX\n"
                                              "attribute vec2 a_position;\n"
                                              "varying vec2 v_uv;\n"
                                              "uniform int flipY;\n"
                                              "void main() {\n"
                                              "	v_uv = a_position * 0.5 + 0.5;\n"
                                              "	if (flipY != 0) { v_uv.y = 1.0 - v_uv.y; }\n"
                                              "	gl_Position = vec4(a_position, 0.0, 1.0);\n"
                                              "}\n"
                                              "#endif\n"
                                              "#ifdef FRAGMENT\n"
                                              "varying vec2 v_uv;\n"
                                              "uniform sampler2D s_src;\n"
                                              "void main() {\n"
                                              "	ivec4 c = ivec4(texture2D(s_src, v_uv) * 255.0 + 0.5);\n"
                                              "	int px16 = (c.r >> 3) | ((c.g >> 3) << 5) | ((c.b >> 3) << 10) | ((c.a >> 7) << 15);\n"
                                              "	fragColor = vec4(float(px16 & 0xFF) / 255.0, float((px16 >> 8) & 0xFF) / 255.0, 0.0, 0.0);\n"
                                              "}\n"
                                              "#endif\n";

// NOTE(aalhendi): Expand packed VRAM without losing bit 15. Internal render
// targets carry that PS1 STP/mask bit in alpha so packing them is lossless.
global_variable const char *ctr_present_vram_shader = "#ifdef VERTEX\n"
                                                      "attribute vec2 a_position;\n"
                                                      "varying vec2 v_uv;\n"
                                                      "uniform vec4 sourceRect;\n"
                                                      "void main() {\n"
                                                      "\tvec2 screenUV = a_position * 0.5 + 0.5;\n"
                                                      "\tvec2 sourcePixel = sourceRect.xy + vec2(screenUV.x, 1.0 - screenUV.y) * sourceRect.zw;\n"
                                                      "\tv_uv = sourcePixel / vec2(1024.0, 512.0);\n"
                                                      "\tgl_Position = vec4(a_position, 0.0, 1.0);\n"
                                                      "}\n"
                                                      "#endif\n"
                                                      "#ifdef FRAGMENT\n"
                                                      "varying vec2 v_uv;\n"
                                                      "uniform sampler2D s_texture;\n"
                                                      "void main() {\n"
                                                      "\tivec2 packedBytes = ivec2(texture2D(s_texture, v_uv).rg * 255.0 + 0.5);\n"
                                                      "\tint pixel = packedBytes.r | (packedBytes.g << 8);\n"
                                                      "\tivec3 color5 = ivec3(pixel & 31, (pixel >> 5) & 31, (pixel >> 10) & 31);\n"
                                                      "\tivec3 color8 = (color5 << 3) | (color5 >> 2);\n"
                                                      "\tfloat stp = float((pixel >> 15) & 1);\n"
                                                      "\tfragColor = vec4(vec3(color8) / 255.0, stp);\n"
                                                      "}\n"
                                                      "#endif\n";

internal void NativeRenderer_InitVRAMPipelines(void)
{
	local_persist const float quad[12] = {-1.f, -1.f, -1.f, 1.f, 1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f, -1.f};

	s_packShader = NativeRenderer_Shader_Compile(ctr_pack_shader, false);
	glUseProgram(s_packShader);
	const GLint packSrcLoc = glGetUniformLocation(s_packShader, "s_src");
	s_packFlipYLoc = glGetUniformLocation(s_packShader, "flipY");
	glUniform1i(packSrcLoc, 0);
	glUseProgram(0);

	s_presentVramShader = NativeRenderer_Shader_Compile(ctr_present_vram_shader, false);
	s_presentVramSourceRectLoc = glGetUniformLocation(s_presentVramShader, "sourceRect");

	glGenVertexArrays(1, &s_vramQuadVAO);
	glGenBuffers(1, &s_vramQuadVBO);
	glBindVertexArray(s_vramQuadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, s_vramQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glEnableVertexAttribArray(a_position);
	glVertexAttribPointer(a_position, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

internal void NativeRenderer_InitRG8LUT(void)
{
	for (u16 y = 0; y < LUT_HEIGHT; y++)
	{
		u8 *row = rgLUT + y * (LUT_WIDTH * 4);
		for (u16 x = 0; x < LUT_WIDTH; x++)
		{
			const u16 c = (y << 8) | x;
			u8 *pixel = row + x * 4;
			pixel[0] = (u8)((c & 31) << 3);
			pixel[1] = (u8)(((c >> 5) & 31) << 3);
			pixel[2] = (u8)(((c >> 10) & 31) << 3);
			pixel[3] = (u8)(((c >> 15) & 1) << 7);
		}
	}
}

int NativeRenderer_InitialisePSX(void)
{
	SDL_memset(s_vram.cpuPixels, 0, sizeof(s_vram.cpuPixels));
	s_vram.cpuDirtyRects[0].x = 0;
	s_vram.cpuDirtyRects[0].y = 0;
	s_vram.cpuDirtyRects[0].w = VRAM_WIDTH;
	s_vram.cpuDirtyRects[0].h = VRAM_HEIGHT;
	s_vram.cpuDirtyRectCount = 1;
	SDL_memset(s_vram.gpuNewerTiles, 0, sizeof(s_vram.gpuNewerTiles));
	NativeRenderer_InitRG8LUT();
	NativeRenderer_GenerateCommonTextures();
	NativeRenderer_InitialisePSXShaders();
	NATIVE_RENDERER_LOG("%s", "*Compiling VRAM pipelines\n");
	NativeRenderer_InitVRAMPipelines();
	NATIVE_RENDERER_LOG("%s", "*VRAM pipelines ready\n");

#if defined(CTR_NATIVE_GPU_TIMERS)
	GLint glMajor = 0;
	GLint glMinor = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &glMajor);
	glGetIntegerv(GL_MINOR_VERSION, &glMinor);
	s_gpuTimerSupported = (glMajor > 3) || ((glMajor == 3) && (glMinor >= 3)) || SDL_GL_ExtensionSupported("GL_ARB_timer_query");
	if (s_gpuTimerSupported)
	{
		GLuint queryIds[NATIVE_GPU_TIMER_QUERY_COUNT];
		glGenQueries(NATIVE_GPU_TIMER_QUERY_COUNT, queryIds);
		for (s32 i = 0; i < NATIVE_GPU_TIMER_QUERY_COUNT; i++)
		{
			s_gpuTimerQueries[i].id = queryIds[i];
		}
	}
#endif

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_STENCIL_TEST);
	glBlendColor(0.5f, 0.5f, 0.5f, 0.25f);

	// Main and offscreen draws share one explicit render-target contract. The
	// main target stays at CTR's logical display size; host scaling is deferred
	// to presentation.
	NativeRenderer_InitRenderTarget(&s_mainRenderTarget);
	NativeRenderer_InitRenderTarget(&s_offscreenRenderTarget);
	NativeRenderer_InitRenderTarget(&s_presentResolveTarget);

	// gen VRAM texture (single, persistent - mirrors PS1's single 1MB VRAM)
	{
		glGenTextures(1, &s_vram.texture);

		glBindTexture(GL_TEXTURE_2D, s_vram.texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		// set storage size
		glTexImage2D(GL_TEXTURE_2D, 0, VRAM_INTERNAL_FORMAT, VRAM_WIDTH, VRAM_HEIGHT, 0, VRAM_FORMAT, GL_UNSIGNED_BYTE, NULL);

		glBindTexture(GL_TEXTURE_2D, 0);

		// VRAM framebuffer for offscreen blitting to VRAM
		glGenFramebuffers(1, &s_glVramFramebuffer);
		{
			glBindFramebuffer(GL_FRAMEBUFFER, s_glVramFramebuffer);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_vram.texture, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

			NativeRenderer_BindPresentFramebuffer();
		}
	}

	// gen vertex buffer and index buffer
	{
		int i;

		glGenBuffers(MAX_NUM_VERTEX_BUFFERS, s_glVertexBuffer);
		glGenVertexArrays(MAX_NUM_VERTEX_BUFFERS, s_glVertexArray);

		for (i = 0; i < MAX_NUM_VERTEX_BUFFERS; i++)
		{
			glBindVertexArray(s_glVertexArray[i]);

			glBindBuffer(GL_ARRAY_BUFFER, s_glVertexBuffer[i]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(GrVertex) * MAX_VERTEX_BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);

			glEnableVertexAttribArray(a_position);
			glEnableVertexAttribArray(a_texcoord);
			glEnableVertexAttribArray(a_color);
			glEnableVertexAttribArray(a_extra);

			glVertexAttribPointer(a_position, 4, GL_SHORT, GL_FALSE, sizeof(GrVertex),
			                      (const void *)(uintptr_t)offsetof(GrVertex, x));
			glVertexAttribPointer(a_texcoord, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(GrVertex),
			                      (const void *)(uintptr_t)offsetof(GrVertex, u));
			glVertexAttribPointer(a_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GrVertex),
			                      (const void *)(uintptr_t)offsetof(GrVertex, r));
			glVertexAttribPointer(a_extra, 4, GL_BYTE, GL_FALSE, sizeof(GrVertex),
			                      (const void *)(uintptr_t)offsetof(GrVertex, tcx));
		}

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	NativeRenderer_ResetDevice();

	return 1;
}

internal void NativeRenderer_Ortho2D(float left, float right, float bottom, float top, float znear, float zfar)
{
	float a = 2.0f / (right - left);
	float b = 2.0f / (top - bottom);
	float c = 2.0f / (znear - zfar);

	float x = (left + right) / (left - right);
	float y = (bottom + top) / (bottom - top);

	// -1..1
	float z = (znear + zfar) / (znear - zfar);

	float ortho[16] = {a, 0, 0, 0, 0, b, 0, 0, 0, 0, c, 0, x, y, z, 1};

	glUniformMatrix4fv(u_projectionLoc, 1, GL_FALSE, ortho);
}

void NativeRenderer_SetupClipMode(const RECT16 *rect, const DISPENV *displayEnv, int enable)
{
	if ((displayEnv->disp.w <= 0) || (displayEnv->disp.h <= 0))
	{
		NativeRenderer_SetScissorState(0);
		return;
	}

	if ((rect->w <= 0) || (rect->h <= 0))
	{
		// NOTE(aalhendi): Retail draw-area commands define inclusive corners.
		// Collapsed areas clip all pixels; GL scissor rejects negative sizes.
		NativeRenderer_SetScissorState(enable != 0);
		if (enable)
		{
			glScissor(0, 0, 0, 0);
		}
		return;
	}

	// [A] isinterlaced dirty hack for widescreen
	const bool scissorOn = enable && (displayEnv->isinter || (rect->x - displayEnv->disp.x > 0 || rect->y - displayEnv->disp.y > 0 ||
	                                                          rect->w < displayEnv->disp.w || rect->h < displayEnv->disp.h));

	NativeRenderer_SetScissorState(scissorOn);

	if (!scissorOn)
	{
		return;
	}

	const float emuScreenAspect = 1.0f;

	const float psxScreenWInv = 1.0f / (float)displayEnv->disp.w;
	const float psxScreenHInv = 1.0f / (float)displayEnv->disp.h;

	// first map to 0..1
	float clipRectX = (float)(rect->x - displayEnv->disp.x) * psxScreenWInv;
	float clipRectY = (float)(rect->y - displayEnv->disp.y) * psxScreenHInv;
	float clipRectW = (float)(rect->w) * psxScreenWInv;
	float clipRectH = (float)(rect->h) * psxScreenHInv;

	// then map to screen
	{
		clipRectX -= 0.5f;

		clipRectX *= emuScreenAspect;
		clipRectW *= emuScreenAspect;

		clipRectX += 0.5f;
	}

	// Normal game draws target the PSX-sized main framebuffer. Host-window
	// coordinates are introduced only by the final presentation pass.
	const float viewportX = 0.0f;
	const float viewportY = 0.0f;
	const float renderScale = (float)s_internalResolutionScale;
	const float viewportW = (float)displayEnv->disp.w * renderScale;
	const float viewportH = (float)displayEnv->disp.h * renderScale;
	const float flipOffset = viewportY + viewportH - clipRectH * viewportH;
	const float crx = viewportX + clipRectX * viewportW;
	const float cry = clipRectY * viewportH;
	const float crw = clipRectW * viewportW;
	const float crh = clipRectH * viewportH;

	glScissor((GLint)crx, (GLint)(flipOffset - cry), (GLsizei)crw, (GLsizei)crh);
}

internal void NativeRenderer_SetShader(const ShaderID shader)
{
	if (s_previousShader != shader)
	{
		glUseProgram(shader);

		s_previousShader = shader;
	}
}


void NativeRenderer_SetTexture(TextureID texture, TexFormat texFormat)
{
	switch (texFormat)
	{
	case TF_4_BIT:
		NativeRenderer_SetShader(s_gteShader4.shader);
		u_bilinearFilterLoc = s_gteShader4.bilinearFilterLoc;
		u_projectionLoc = s_gteShader4.projectionLoc;
		u_texelSizeLoc = -1;
		u_psxSemiTransPassLoc = s_gteShader4.psxSemiTransPassLoc;
		u_psxFramebufferFetchBlendModeLoc = s_gteShader4.psxFramebufferFetchBlendModeLoc;
		u_psxDrawMaskSetLoc = s_gteShader4.psxDrawMaskSetLoc;
		u_psxTextureOutputStpLoc = s_gteShader4.psxTextureOutputStpLoc;
		break;
	case TF_8_BIT:
		NativeRenderer_SetShader(s_gteShader8.shader);
		u_bilinearFilterLoc = s_gteShader8.bilinearFilterLoc;
		u_projectionLoc = s_gteShader8.projectionLoc;
		u_texelSizeLoc = -1;
		u_psxSemiTransPassLoc = s_gteShader8.psxSemiTransPassLoc;
		u_psxFramebufferFetchBlendModeLoc = s_gteShader8.psxFramebufferFetchBlendModeLoc;
		u_psxDrawMaskSetLoc = s_gteShader8.psxDrawMaskSetLoc;
		u_psxTextureOutputStpLoc = s_gteShader8.psxTextureOutputStpLoc;
		break;
	case TF_16_BIT:
		NativeRenderer_SetShader(s_gteShader16.shader);
		u_bilinearFilterLoc = s_gteShader16.bilinearFilterLoc;
		u_projectionLoc = s_gteShader16.projectionLoc;
		u_texelSizeLoc = -1;
		u_psxSemiTransPassLoc = s_gteShader16.psxSemiTransPassLoc;
		u_psxFramebufferFetchBlendModeLoc = s_gteShader16.psxFramebufferFetchBlendModeLoc;
		u_psxDrawMaskSetLoc = s_gteShader16.psxDrawMaskSetLoc;
		u_psxTextureOutputStpLoc = s_gteShader16.psxTextureOutputStpLoc;
		break;
	case TF_32_BIT_RGBA:
		NativeRenderer_SetShader(s_gteShader32Rgba.shader);
		u_bilinearFilterLoc = s_gteShader32Rgba.bilinearFilterLoc;
		u_projectionLoc = s_gteShader32Rgba.projectionLoc;
		u_texelSizeLoc = s_gteShader32Rgba.texelSizeLoc;
		u_psxSemiTransPassLoc = s_gteShader32Rgba.psxSemiTransPassLoc;
		u_psxFramebufferFetchBlendModeLoc = s_gteShader32Rgba.psxFramebufferFetchBlendModeLoc;
		u_psxDrawMaskSetLoc = s_gteShader32Rgba.psxDrawMaskSetLoc;
		u_psxTextureOutputStpLoc = s_gteShader32Rgba.psxTextureOutputStpLoc;
		break;
	}

	if (g_dbg_texturelessMode)
	{
		texture = s_whiteTexture;
	}

	// NOTE(penta3): s_texture (unit 0) and s_rgLut (unit 1) sampler bindings are baked
	// into each program at compile time (NativeRenderer_Shader_Compile) and uniform
	// values persist per-program, so re-setting them on every split was redundant GL
	// churn. bilinearFilter stays here because it toggles at runtime (debug key).
	if (u_bilinearFilterLoc >= 0)
	{
		glUniform1i(u_bilinearFilterLoc, g_cfg_bilinearFiltering);
	}
	NativeRenderer_SetPSXTextureSemiTransPass(0);

	if (s_lastBoundTexture == texture)
	{
		return;
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	s_lastBoundTexture = texture;
}

void NativeRenderer_SetOverrideTextureSize(int width, int height)
{
	if (u_texelSizeLoc == -1)
	{
		return;
	}

	float vec[] = {1.0f / (float)width, 1.0f / (float)height};
	glUniform2fv(u_texelSizeLoc, 1, vec);
}

void NativeRenderer_SetPSXTextureSemiTransPass(int pass)
{
	if (u_psxSemiTransPassLoc >= 0)
	{
		glUniform1i(u_psxSemiTransPassLoc, pass);
	}
}

void NativeRenderer_SetPSXFramebufferFetchBlendMode(int blendMode)
{
	if (u_psxFramebufferFetchBlendModeLoc >= 0)
	{
		glUniform1i(u_psxFramebufferFetchBlendModeLoc, blendMode);
	}
}

int NativeRenderer_UsesFramebufferFetch(void)
{
	return s_psxFramebufferFetchEnabled && (u_psxFramebufferFetchBlendModeLoc >= 0);
}

void NativeRenderer_SetPSXTextureOutputSTP(int enabled)
{
	if (u_psxTextureOutputStpLoc >= 0)
	{
		glUniform1i(u_psxTextureOutputStpLoc, enabled);
	}
}

void NativeRenderer_SetPSXDrawMaskSet(int maskSet)
{
	if (u_psxDrawMaskSetLoc >= 0)
	{
		glUniform1i(u_psxDrawMaskSetLoc, maskSet);
	}
}

internal void NativeRenderer_DestroyTexture(TextureID texture)
{
	if (texture == (TextureID)-1)
	{
		return;
	}

	glDeleteTextures(1, &texture);
}

internal u16 NativeRenderer_PackRGB24ToPSX15(u8 r, u8 g, u8 b)
{
	return (u16)(((r >> 3) & 0x1f) | (((g >> 3) & 0x1f) << 5) | (((b >> 3) & 0x1f) << 10));
}

internal float NativeRenderer_PSXColorComponentFloat(u8 value)
{
	const u8 psx8 = (u8)((value >> 3) << 3);
	return (float)psx8 / 255.0f;
}

internal int NativeRenderer_ClipVRAMRect(RECT16 *out, int x, int y, int w, int h)
{
	if ((w <= 0) || (h <= 0))
	{
		return 0;
	}

	if (x < 0)
	{
		w += x;
		x = 0;
	}
	if (y < 0)
	{
		h += y;
		y = 0;
	}
	if (x + w > VRAM_WIDTH)
	{
		w = VRAM_WIDTH - x;
	}
	if (y + h > VRAM_HEIGHT)
	{
		h = VRAM_HEIGHT - y;
	}
	if ((w <= 0) || (h <= 0))
	{
		return 0;
	}

	out->x = (s16)x;
	out->y = (s16)y;
	out->w = (s16)w;
	out->h = (s16)h;
	return 1;
}

internal int NativeRenderer_DirtyRectContains(const RECT16 *outer, const RECT16 *inner)
{
	const int outerRight = outer->x + outer->w;
	const int outerBottom = outer->y + outer->h;
	const int innerRight = inner->x + inner->w;
	const int innerBottom = inner->y + inner->h;

	return (outer->x <= inner->x) && (outer->y <= inner->y) && (outerRight >= innerRight) && (outerBottom >= innerBottom);
}

internal int NativeRenderer_TryMergeDirtyRect(RECT16 *dst, const RECT16 *src)
{
	const int dstRight = dst->x + dst->w;
	const int dstBottom = dst->y + dst->h;
	const int srcRight = src->x + src->w;
	const int srcBottom = src->y + src->h;

	if ((dst->y == src->y) && (dst->h == src->h) && (src->x <= dstRight) && (dst->x <= srcRight))
	{
		const int x0 = (src->x < dst->x) ? src->x : dst->x;
		const int x1 = (srcRight > dstRight) ? srcRight : dstRight;
		dst->x = (s16)x0;
		dst->w = (s16)(x1 - x0);
		return 1;
	}

	if ((dst->x == src->x) && (dst->w == src->w) && (src->y <= dstBottom) && (dst->y <= srcBottom))
	{
		const int y0 = (src->y < dst->y) ? src->y : dst->y;
		const int y1 = (srcBottom > dstBottom) ? srcBottom : dstBottom;
		dst->y = (s16)y0;
		dst->h = (s16)(y1 - y0);
		return 1;
	}

	return 0;
}

internal void NativeRenderer_AppendCpuDirtyRect(const RECT16 *rect)
{
	for (s32 i = 0; i < s_vram.cpuDirtyRectCount; i++)
	{
		if (NativeRenderer_DirtyRectContains(&s_vram.cpuDirtyRects[i], rect))
		{
			return;
		}

		if (NativeRenderer_DirtyRectContains(rect, &s_vram.cpuDirtyRects[i]))
		{
			s_vram.cpuDirtyRects[i] = *rect;
			return;
		}

		if (NativeRenderer_TryMergeDirtyRect(&s_vram.cpuDirtyRects[i], rect))
		{
			return;
		}
	}

	if (s_vram.cpuDirtyRectCount >= NATIVE_VRAM_DIRTY_RECT_CAP)
	{
		NativeRenderer_UpdateVRAM();
	}

	if (s_vram.cpuDirtyRectCount < NATIVE_VRAM_DIRTY_RECT_CAP)
	{
		s_vram.cpuDirtyRects[s_vram.cpuDirtyRectCount] = *rect;
		s_vram.cpuDirtyRectCount++;
	}
}

internal void NativeRenderer_MarkVRAMDirty(int x, int y, int w, int h)
{
	RECT16 rect;

	if (!NativeRenderer_ClipVRAMRect(&rect, x, y, w, h))
	{
		return;
	}

	NativeRenderer_AppendCpuDirtyRect(&rect);
}

internal void NativeRenderer_MarkGpuVRAMNewer(int x, int y, int w, int h)
{
	RECT16 rect;

	if (!NativeRenderer_ClipVRAMRect(&rect, x, y, w, h))
	{
		return;
	}

	const int tileX0 = rect.x / NATIVE_VRAM_TILE_SIZE;
	const int tileY0 = rect.y / NATIVE_VRAM_TILE_SIZE;
	const int tileX1 = (rect.x + rect.w - 1) / NATIVE_VRAM_TILE_SIZE;
	const int tileY1 = (rect.y + rect.h - 1) / NATIVE_VRAM_TILE_SIZE;

	for (int tileY = tileY0; tileY <= tileY1; tileY++)
	{
		for (int tileX = tileX0; tileX <= tileX1; tileX++)
		{
			const int tileIndex = tileY * NATIVE_VRAM_TILE_COLS + tileX;
			s_vram.gpuNewerTiles[tileIndex >> 5] |= 1u << (tileIndex & 31);
		}
	}
}

void NativeRenderer_ClearVRAM(int x, int y, int w, int h, u8 r, u8 g, u8 b)
{
	u16 *dst = s_vram.cpuPixels + x + y * VRAM_WIDTH;
	const u16 color = NativeRenderer_PackRGB24ToPSX15(r, g, b);

	if (x + w > VRAM_WIDTH)
	{
		w = VRAM_WIDTH - x;
	}

	if (y + h > VRAM_HEIGHT)
	{
		h = VRAM_HEIGHT - y;
	}

	// clear VRAM region with given color
	for (int i = 0; i < h; i++)
	{
		u16 *tmp = dst;

		for (int j = 0; j < w; j++)
		{
			*tmp++ = color;
		}

		dst += VRAM_WIDTH;
	}

	NativeRenderer_MarkVRAMDirty(x, y, w, h);
}

void NativeRenderer_Clear(int x, int y, int w, int h, u8 r, u8 g, u8 b)
{
	if ((w <= 0) || (h <= 0))
	{
		return;
	}

	int displayX = activeDispEnv.disp.x;
	int displayY = activeDispEnv.disp.y;
	int displayW = activeDispEnv.disp.w;
	int displayH = activeDispEnv.disp.h;

	if ((displayW <= 0) || (displayH <= 0))
	{
		displayX = activeDrawEnv.clip.x;
		displayY = activeDrawEnv.clip.y;
		displayW = activeDrawEnv.clip.w;
		displayH = activeDrawEnv.clip.h;
	}

	if ((displayW <= 0) || (displayH <= 0))
	{
		return;
	}

	const int clearRight = x + w;
	const int clearBottom = y + h;
	const int displayRight = displayX + displayW;
	const int displayBottom = displayY + displayH;

	const int overlapX = x > displayX ? x : displayX;
	const int overlapY = y > displayY ? y : displayY;
	const int overlapRight = clearRight < displayRight ? clearRight : displayRight;
	const int overlapBottom = clearBottom < displayBottom ? clearBottom : displayBottom;

	if ((overlapRight <= overlapX) || (overlapBottom <= overlapY))
	{
		return;
	}

	const int relX = overlapX - displayX;
	const int relBottom = overlapBottom - displayY;
	const int scissorX = relX * s_internalResolutionScale;
	const int scissorY = (displayH - relBottom) * s_internalResolutionScale;
	const int scissorW = (overlapRight - overlapX) * s_internalResolutionScale;
	const int scissorH = (overlapBottom - overlapY) * s_internalResolutionScale;

	if ((scissorW <= 0) || (scissorH <= 0))
	{
		return;
	}

	GLint previousScissorBox[4];
	const GLboolean previousScissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
	glGetIntegerv(GL_SCISSOR_BOX, previousScissorBox);

	glEnable(GL_SCISSOR_TEST);
	glScissor(scissorX, scissorY, scissorW, scissorH);
	glClearColor(NativeRenderer_PSXColorComponentFloat(r), NativeRenderer_PSXColorComponentFloat(g), NativeRenderer_PSXColorComponentFloat(b), 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	if (previousScissorEnabled)
	{
		glEnable(GL_SCISSOR_TEST);
		glScissor(previousScissorBox[0], previousScissorBox[1], previousScissorBox[2], previousScissorBox[3]);
	}
	else
	{
		glDisable(GL_SCISSOR_TEST);
	}

	s_previousScissorState = previousScissorEnabled ? 1 : 0;
}

void NativeRenderer_SaveVRAM(const char *outputFileName, int x, int y, int width, int height, int bReadFromFrameBuffer)
{
#define FLIP_Y (VRAM_HEIGHT - i - 1)

	(void)x;
	(void)y;
	(void)bReadFromFrameBuffer;

	NativeRenderer_SyncGpuVRAMToCPU(0, 0, VRAM_WIDTH, VRAM_HEIGHT);

	FILE *fp = fopen(outputFileName, "wb");
	if (fp == NULL)
	{
		return;
	}

	u8 TGAheader[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	u8 header[6];
	header[0] = (width % 256);
	header[1] = (width / 256);
	header[2] = (height % 256);
	header[3] = (height / 256);
	header[4] = 16;
	header[5] = 0;

	fwrite(TGAheader, sizeof(u8), 12, fp);
	fwrite(header, sizeof(u8), 6, fp);

	for (int i = 0; i < VRAM_HEIGHT; i++)
	{
		fwrite(s_vram.cpuPixels + VRAM_WIDTH * FLIP_Y, sizeof(u16), VRAM_WIDTH, fp);
	}

	fclose(fp);

#undef FLIP_Y
}

internal void NativeRenderer_SyncGpuVRAMToCPU(int x, int y, int w, int h)
{
	RECT16 readRect;

	if (!NativeRenderer_ClipVRAMRect(&readRect, x, y, w, h))
	{
		return;
	}

	const int tileX0 = readRect.x / NATIVE_VRAM_TILE_SIZE;
	const int tileY0 = readRect.y / NATIVE_VRAM_TILE_SIZE;
	const int tileX1 = (readRect.x + readRect.w - 1) / NATIVE_VRAM_TILE_SIZE;
	const int tileY1 = (readRect.y + readRect.h - 1) / NATIVE_VRAM_TILE_SIZE;
	b32 needsReadback = false;

	for (int tileY = tileY0; tileY <= tileY1 && !needsReadback; tileY++)
	{
		for (int tileX = tileX0; tileX <= tileX1; tileX++)
		{
			const int tileIndex = tileY * NATIVE_VRAM_TILE_COLS + tileX;
			if ((s_vram.gpuNewerTiles[tileIndex >> 5] & (1u << (tileIndex & 31))) != 0)
			{
				needsReadback = true;
				break;
			}
		}
	}

	if (!needsReadback)
	{
		return;
	}

	readRect.x = (s16)(tileX0 * NATIVE_VRAM_TILE_SIZE);
	readRect.y = (s16)(tileY0 * NATIVE_VRAM_TILE_SIZE);
	readRect.w = (s16)((tileX1 - tileX0 + 1) * NATIVE_VRAM_TILE_SIZE);
	readRect.h = (s16)((tileY1 - tileY0 + 1) * NATIVE_VRAM_TILE_SIZE);

	// CPU writes must reach the persistent texture before a GPU-authored region
	// is read back, preserving PS1 VRAM command order in the split host mirror.
	NativeRenderer_UpdateVRAM();

	NativePerf_BeginScope(NATIVE_PERF_BUCKET_FRAMEBUFFER_READBACK);
	GLint previousReadFramebuffer;
	GLint previousPackRowLength;
	GLint previousPackAlignment;
	b32 readbackSucceeded = true;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
	glGetIntegerv(GL_PACK_ROW_LENGTH, &previousPackRowLength);
	glGetIntegerv(GL_PACK_ALIGNMENT, &previousPackAlignment);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, s_glVramFramebuffer);

#if defined(CTR_NATIVE_RENDERER_GLES)
	// GLES 3 guarantees RGBA/UNSIGNED_BYTE readback, but not GL_RG readback
	// from an RG8 attachment. Apple GLES rejects GL_RG with GL_INVALID_OPERATION.
	// Read the two packed VRAM bytes through guaranteed RGBA and discard B/A.
	const size_t readbackPixelCount = (size_t)readRect.w * readRect.h;
	u8 *readback = (u8 *)SDL_malloc(readbackPixelCount * 4);
	if (readback == NULL)
	{
		NATIVE_RENDERER_ERROR("GLES VRAM readback allocation failed for %dx%d\n", readRect.w, readRect.h);
		readbackSucceeded = false;
	}
	else
	{
		glPixelStorei(GL_PACK_ROW_LENGTH, 0);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(readRect.x, readRect.y, readRect.w, readRect.h, GL_RGBA, GL_UNSIGNED_BYTE, readback);
		const GLenum readbackError = glGetError();
		if (readbackError != GL_NO_ERROR)
		{
			NATIVE_RENDERER_ERROR("GLES VRAM RGBA readback failed: error=0x%x rect=(%d,%d %dx%d)\n", readbackError, readRect.x,
			                      readRect.y, readRect.w, readRect.h);
			readbackSucceeded = false;
		}
		else
		{
			for (int row = 0; row < readRect.h; row++)
			{
				u16 *dst = s_vram.cpuPixels + (size_t)(readRect.y + row) * VRAM_WIDTH + readRect.x;
				const u8 *src = readback + (size_t)row * readRect.w * 4;
				for (int column = 0; column < readRect.w; column++)
				{
					dst[column] = (u16)(src[column * 4] | ((u16)src[column * 4 + 1] << 8));
				}
			}
		}
		SDL_free(readback);
	}
#else
	glPixelStorei(GL_PACK_ROW_LENGTH, VRAM_WIDTH);
	glPixelStorei(GL_PACK_ALIGNMENT, sizeof(u16));
	glReadPixels(readRect.x, readRect.y, readRect.w, readRect.h, VRAM_FORMAT, GL_UNSIGNED_BYTE,
	             s_vram.cpuPixels + (size_t)readRect.y * VRAM_WIDTH + readRect.x);
	const GLenum readbackError = glGetError();
	if (readbackError != GL_NO_ERROR)
	{
		NATIVE_RENDERER_ERROR("GL VRAM RG readback failed: error=0x%x rect=(%d,%d %dx%d)\n", readbackError, readRect.x, readRect.y,
		                      readRect.w, readRect.h);
		readbackSucceeded = false;
	}
#endif

	for (int tileY = tileY0; readbackSucceeded && tileY <= tileY1; tileY++)
	{
		for (int tileX = tileX0; tileX <= tileX1; tileX++)
		{
			const int tileIndex = tileY * NATIVE_VRAM_TILE_COLS + tileX;
			s_vram.gpuNewerTiles[tileIndex >> 5] &= ~(1u << (tileIndex & 31));
		}
	}

	glPixelStorei(GL_PACK_ROW_LENGTH, previousPackRowLength);
	glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previousReadFramebuffer);
	NativePerf_EndScope(NATIVE_PERF_BUCKET_FRAMEBUFFER_READBACK);
}

internal void NativeRenderer_ResolveVRAMRead(int x, int y, int w, int h)
{
	NativeRenderer_SyncGpuVRAMToCPU(x, y, w, h);
}

internal int NativeRenderer_RectEquals(const RECT16 *a, const RECT16 *b)
{
	return a->x == b->x && a->y == b->y && a->w == b->w && a->h == b->h;
}

internal void NativeRenderer_FlushOffscreenToVRAM(void)
{
	if (s_previousOffscreen.w <= 0 || s_previousOffscreen.h <= 0)
	{
		return;
	}

	// NOTE(aalhendi): Native offscreen draws produce RGBA pixels. Pack them into
	// the persistent 5:5:5:1 VRAM texture instead of reading them through the CPU.
	NativeRenderer_GpuPackTextureToVRAM(s_offscreenRenderTarget.texture, s_previousOffscreen.x, s_previousOffscreen.y, s_previousOffscreen.w,
	                                    s_previousOffscreen.h, true);
}

internal void NativeRenderer_SetScissorState(int enable)
{
	if (s_previousScissorState == enable)
	{
		return;
	}

	if (s_previousScissorState)
	{
		glDisable(GL_SCISSOR_TEST);
	}
	else
	{
		glEnable(GL_SCISSOR_TEST);
	}
	s_previousScissorState = enable;
}

void NativeRenderer_SetOffscreenState(const RECT16 *offscreenRect, int enable)
{
	const int sameOffscreenRect = NativeRenderer_RectEquals(&s_previousOffscreen, offscreenRect);
	if (!enable && !s_previousOffscreenState)
	{
		return;
	}

	if (enable && s_previousOffscreenState && sameOffscreenRect)
	{
		return;
	}

	if (enable)
	{
		if (s_previousOffscreenState)
		{
			NativeRenderer_FlushOffscreenToVRAM();
		}

		s_previousOffscreenState = 1;
		NativeRenderer_EnsureRenderTarget(&s_offscreenRenderTarget, offscreenRect->w, offscreenRect->h);
		s_previousOffscreen = *offscreenRect;
		NativeRenderer_LoadRenderTargetFromVRAM(&s_offscreenRenderTarget, offscreenRect->x, offscreenRect->y,
		                                        offscreenRect->w, offscreenRect->h);
	}
	else
	{
		s_previousOffscreenState = 0;

		NativeRenderer_FlushOffscreenToVRAM();
		NativeRenderer_BindMainRenderTarget();
		NativeRenderer_SetViewPort(0, 0, s_mainRenderTarget.width, s_mainRenderTarget.height);
	}
}

void NativeRenderer_SetProjection(const RECT16 *drawRect, const DISPENV *displayEnv, int offscreen)
{
	if (offscreen)
	{
		NativeRenderer_Ortho2D(0, drawRect->w, drawRect->h, 0, -1.0f, 1.0f);
		return;
	}

	const int displayW = displayEnv->disp.w > 0 ? displayEnv->disp.w : 1;
	const int displayH = displayEnv->disp.h > 0 ? displayEnv->disp.h : 1;
	NativeRenderer_Ortho2D(0, displayW, displayH, 0, -1.0f, 1.0f);
}

// NOTE(aalhendi): Pack an RGBA render texture straight into the RG8 VRAM texture
// on the GPU, no CPU round trip. Restore or invalidate the render-state caches
// disturbed by this native bridge before the submit run continues.
internal void NativeRenderer_GpuPackTextureToVRAM(TextureID sourceTexture, int x, int y, int w, int h, b32 flipY)
{
	const ShaderID previousShader = s_previousShader;
	const TextureID previousTexture = s_lastBoundTexture;
	const BlendMode previousBlendMode = s_previousBlendMode;
	const int previousScissorState = s_previousScissorState;

	NativeRenderer_UpdateVRAM();

	glBindFramebuffer(GL_FRAMEBUFFER, s_glVramFramebuffer);

	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glViewport(x, y, w, h);

	glUseProgram(s_packShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sourceTexture);
	glUniform1i(s_packFlipYLoc, flipY);

	glBindVertexArray(s_vramQuadVAO);
	NativeRenderer_DrawTriangles(0, 2);
	if (s_boundVertexBuffer >= 0)
	{
		glBindVertexArray(s_glVertexArray[s_boundVertexBuffer]);
	}
	else
	{
		glBindVertexArray(0);
	}

	if (s_previousOffscreenState)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, s_offscreenRenderTarget.framebuffer);
		glViewport(0, 0, s_offscreenRenderTarget.width, s_offscreenRenderTarget.height);
	}
	else
	{
		NativeRenderer_BindMainRenderTarget();
		glViewport(0, 0, s_mainRenderTarget.width, s_mainRenderTarget.height);
	}

	glUseProgram(previousShader == (ShaderID)-1 ? 0 : previousShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, previousTexture == (TextureID)-1 ? 0 : previousTexture);
	s_previousShader = previousShader;
	s_lastBoundTexture = previousTexture;
	s_previousBlendMode = BM_NONE;
	s_previousScissorState = 0;
	NativeRenderer_SetBlendMode(previousBlendMode);
	NativeRenderer_SetScissorState(previousScissorState);
	NativeRenderer_MarkGpuVRAMNewer(x, y, w, h);
}

// NOTE(aalhendi): PS1 draws into VRAM and can texture from that same VRAM. Native
// mirrors that by flushing pending CPU VRAM writes, then packing the presented
// framebuffer into the persistent RG8 VRAM texture. CPU-side VRAM reads pull from
// that packed texture lazily, avoiding the old per-frame GPU->CPU->GPU round trip.
void NativeRenderer_StoreFrameBuffer(int x, int y, int w, int h)
{
	NativePerf_BeginScope(NATIVE_PERF_BUCKET_FRAMEBUFFER_STORE);

	NativeRenderer_GpuPackTextureToVRAM(s_mainRenderTarget.texture, x, y, w, h, true);

	NativePerf_EndScope(NATIVE_PERF_BUCKET_FRAMEBUFFER_STORE);
}

void NativeRenderer_PresentMainRenderTarget(void)
{
	if ((s_internalResolutionScale <= 1) || (s_mainRenderTarget.width <= 0) || (s_mainRenderTarget.height <= 0))
	{
		NativeRenderer_PresentVRAMDisplay();
		return;
	}

	NativePerf_BeginScope(NATIVE_PERF_BUCKET_RENDERER_PRESENT_VRAM);
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, s_mainRenderTarget.framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_presentFramebuffer);
	glBlitFramebuffer(0, 0, s_mainRenderTarget.width, s_mainRenderTarget.height,
	                  s_presentViewport.x, s_presentViewport.y,
	                  s_presentViewport.x + s_presentViewport.w, s_presentViewport.y + s_presentViewport.h,
	                  GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, s_presentFramebuffer);
	glEnable(GL_STENCIL_TEST);
	glBindVertexArray(0);

	s_previousBlendMode = BM_NONE;
	s_previousScissorState = 0;
	s_previousShader = (ShaderID)-1;
	s_lastBoundTexture = (TextureID)-1;
	NativePerf_EndScope(NATIVE_PERF_BUCKET_RENDERER_PRESENT_VRAM);
}

void NativeRenderer_CopyVRAM(u16 *src, int x, int y, int w, int h, int dst_x, int dst_y)
{
	int stride = w;

	if ((w <= 0) || (h <= 0) || (dst_x < 0) || (dst_y < 0) || (dst_x > VRAM_WIDTH - w) || (dst_y > VRAM_HEIGHT - h) ||
	    ((src == NULL) && ((x < 0) || (y < 0) || (x > VRAM_WIDTH - w) || (y > VRAM_HEIGHT - h))))
	{
		NATIVE_RENDERER_ERROR("invalid VRAM copy src=%p rect=(%d,%d %dx%d) dst=(%d,%d)\n", (void *)src, x, y, w, h, dst_x, dst_y);
		return;
	}

	if (!src)
	{
		// NOTE(aalhendi): MoveImage reads exactly its PS1 VRAM source rectangle. Resolve only that
		// GPU-authored region into the CPU mirror before copying it.
		NativeRenderer_ResolveVRAMRead(x, y, w, h);
		src = s_vram.cpuPixels;
		stride = VRAM_WIDTH;
	}

	src += x + y * stride;

	u16 *dst = s_vram.cpuPixels + dst_x + dst_y * VRAM_WIDTH;

	for (int i = 0; i < h; i++)
	{
		SDL_memcpy(dst, src, w * sizeof(u16));
		dst += VRAM_WIDTH;
		src += stride;
	}

	NativeRenderer_MarkVRAMDirty(dst_x, dst_y, w, h);
}

void NativeRenderer_ReadVRAM(u16 *dst, int x, int y, int dst_w, int dst_h)
{
	NativeRenderer_ResolveVRAMRead(x, y, dst_w, dst_h);

	u16 *src = s_vram.cpuPixels + x + VRAM_WIDTH * y;

	for (int i = 0; i < dst_h; i++)
	{
		SDL_memcpy(dst, src, dst_w * sizeof(u16));
		dst += dst_w;
		src += VRAM_WIDTH;
	}
}

int NativeRenderer_GetVRAMStateSize(void)
{
	return (int)sizeof(s_vram.cpuPixels);
}

int NativeRenderer_CaptureVRAMState(void *dst, int dstSize)
{
	if ((dst == NULL) || (dstSize < (int)sizeof(s_vram.cpuPixels)))
	{
		return 0;
	}

	// NOTE(aalhendi): Save-states own the CPU-side PSX VRAM mirror, not GL
	// textures. Pull pending GPU-authored VRAM into the mirror first.
	NativeRenderer_SyncGpuVRAMToCPU(0, 0, VRAM_WIDTH, VRAM_HEIGHT);
	SDL_memcpy(dst, s_vram.cpuPixels, sizeof(s_vram.cpuPixels));
	return 1;
}

int NativeRenderer_RestoreVRAMState(const void *src, int srcSize)
{
	local_persist const RECT16 zeroRect = {0, 0, 0, 0};

	if ((src == NULL) || (srcSize < (int)sizeof(s_vram.cpuPixels)))
	{
		return 0;
	}

	SDL_memcpy(s_vram.cpuPixels, src, sizeof(s_vram.cpuPixels));
	// NOTE(aalhendi): Restored VRAM is authoritative PSX state. Host GL caches
	// are rebuildable, so mark all of VRAM dirty and drop stale bindings.
	s_vram.cpuDirtyRectCount = 0;
	SDL_memset(s_vram.gpuNewerTiles, 0, sizeof(s_vram.gpuNewerTiles));
	NativeRenderer_MarkVRAMDirty(0, 0, VRAM_WIDTH, VRAM_HEIGHT);
	s_mainRenderTarget.width = 0;
	s_mainRenderTarget.height = 0;
	s_offscreenRenderTarget.width = 0;
	s_offscreenRenderTarget.height = 0;
	s_previousOffscreen = zeroRect;
	s_previousOffscreenState = 0;
	s_previousShader = (ShaderID)-1;
	s_lastBoundTexture = (TextureID)-1;
	return 1;
}

void NativeRenderer_UpdateVRAM(void)
{
	if (s_vram.cpuDirtyRectCount == 0)
	{
		return;
	}

	NativePerf_BeginScope(NATIVE_PERF_BUCKET_RENDERER_UPDATE_VRAM);

	const s32 rectCount = s_vram.cpuDirtyRectCount;
	s_vram.cpuDirtyRectCount = 0;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, s_vram.texture);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, VRAM_WIDTH);
	for (s32 i = 0; i < rectCount; i++)
	{
		const RECT16 r = s_vram.cpuDirtyRects[i];
		glTexSubImage2D(GL_TEXTURE_2D, 0, r.x, r.y, r.w, r.h, VRAM_FORMAT, GL_UNSIGNED_BYTE, s_vram.cpuPixels + (size_t)r.y * VRAM_WIDTH + r.x);
	}
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	s_lastBoundTexture = (TextureID)-1;

	NativePerf_EndScope(NATIVE_PERF_BUCKET_RENDERER_UPDATE_VRAM);
}

internal void NativeRenderer_PresentVRAMRectDirect(int displayX, int displayY, int displayW, int displayH)
{
	if (displayW <= 0 || displayH <= 0)
	{
		return;
	}

	NativeRenderer_UpdateVRAM();

	NativeRenderer_SetViewPort(s_presentViewport.x, s_presentViewport.y, s_presentViewport.w, s_presentViewport.h);
	NativeRenderer_BindPresentFramebuffer();

	NativeRenderer_SetScissorState(0);
	NativeRenderer_EnableDepth(0);
	NativeRenderer_SetBlendMode(BM_NONE);

	NativeRenderer_DrawVRAMRegion(displayX, displayY, displayW, displayH);
	glBindVertexArray(0);

	s_previousShader = (ShaderID)-1;
	s_lastBoundTexture = (TextureID)-1;
}

void NativeRenderer_PresentVRAMRect(int displayX, int displayY, int displayW, int displayH)
{
	if (displayW <= 0 || displayH <= 0)
	{
		return;
	}

	// Decode packed PS1 VRAM once at the logical display size. The old direct
	// path ran the integer unpack shader for every host-window pixel, which is
	// especially expensive under Apple's Simulator software renderer. A nearest
	// framebuffer blit then performs only the scale to the presentation viewport.
	NativePerf_BeginScope(NATIVE_PERF_BUCKET_RENDERER_PRESENT_VRAM);
	NativeRenderer_UpdateVRAM();
	NativeRenderer_EnsureRenderTarget(&s_presentResolveTarget, displayW, displayH);
	glBindFramebuffer(GL_FRAMEBUFFER, s_presentResolveTarget.framebuffer);
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);
	glViewport(0, 0, displayW, displayH);
	NativeRenderer_DrawVRAMRegion(displayX, displayY, displayW, displayH);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, s_presentResolveTarget.framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_presentFramebuffer);
	glBlitFramebuffer(0, 0, displayW, displayH, s_presentViewport.x, s_presentViewport.y,
	                  s_presentViewport.x + s_presentViewport.w, s_presentViewport.y + s_presentViewport.h,
	                  GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, s_presentFramebuffer);
	glEnable(GL_STENCIL_TEST);
	glBindVertexArray(0);

	s_previousBlendMode = BM_NONE;
	s_previousScissorState = 0;
	s_previousShader = (ShaderID)-1;
	s_lastBoundTexture = (TextureID)-1;
	NativePerf_EndScope(NATIVE_PERF_BUCKET_RENDERER_PRESENT_VRAM);
}

int NativeRenderer_CapturePresentedRGBA(u8 *dst, int width, int height)
{
	const size_t rowBytes = (size_t)width * 4;
	GLint previousPackAlignment;
	u8 *swapRow;

	if ((dst == NULL) || (width != g_windowWidth) || (height != g_windowHeight) || (width <= 0) || (height <= 0))
	{
		return 0;
	}

	glGetIntegerv(GL_PACK_ALIGNMENT, &previousPackAlignment);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, dst);
	glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);
	if (glGetError() != GL_NO_ERROR)
	{
		return 0;
	}

	swapRow = (u8 *)SDL_malloc(rowBytes);
	if (swapRow == NULL)
	{
		return 0;
	}

	for (int y = 0; y < height / 2; y++)
	{
		u8 *top = dst + (size_t)y * rowBytes;
		u8 *bottom = dst + (size_t)(height - 1 - y) * rowBytes;

		SDL_memcpy(swapRow, top, rowBytes);
		SDL_memcpy(top, bottom, rowBytes);
		SDL_memcpy(bottom, swapRow, rowBytes);
	}

	SDL_free(swapRow);
	return 1;
}

void NativeRenderer_PresentVRAMDisplay(void)
{
	// NOTE(aalhendi): ctr-native local divergence. Retail presents this boot
	// splash path by displaying VRAM directly after DR_MOVE packets; the native
	// OpenGL backend otherwise swaps the current framebuffer and never shows
	// those VRAM-only copies.
	NativeRenderer_PresentVRAMRect(activeDispEnv.disp.x, activeDispEnv.disp.y, activeDispEnv.disp.w, activeDispEnv.disp.h);
}

void NativeRenderer_SwapWindow(void)
{
	NativePerf_BeginScope(NATIVE_PERF_BUCKET_SWAP_WINDOW);
	if (s_presentRenderbuffer != 0)
	{
		glBindRenderbuffer(GL_RENDERBUFFER, s_presentRenderbuffer);
	}
	if (!SDL_GL_SwapWindow(g_window))
	{
		NATIVE_RENDERER_ERROR("Failed to swap SDL window: %s\n", SDL_GetError());
	}
	NativePerf_EndScope(NATIVE_PERF_BUCKET_SWAP_WINDOW);
}

internal void NativeRenderer_EnableDepth(int enable)
{
	if (s_previousDepthMode == enable)
	{
		return;
	}

	s_previousDepthMode = enable;

	glDisable(GL_DEPTH_TEST);
}

void NativeRenderer_SetStencilMode(int drawPrim)
{
	if (s_previousStencilMode == drawPrim)
	{
		return;
	}

	s_previousStencilMode = drawPrim;

	if (drawPrim)
	{
		glStencilFunc(GL_ALWAYS, 1, 0x10);
		glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	}
	else
	{
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);
	}
}

void NativeRenderer_SetBlendMode(BlendMode blendMode)
{
	if (s_previousBlendMode == blendMode)
	{
		return;
	}

	if (blendMode != BM_NONE)
	{
		if (s_previousBlendMode == BM_NONE)
		{
			glBlendColor(0.25f, 0.25f, 0.25f, 0.5f);
			glEnable(GL_BLEND);
		}

		NativeRenderer_EnableDepth(0);
	}

	switch (blendMode)
	{
	case BM_NONE:
		if (s_previousBlendMode != BM_NONE)
		{
			glBlendColor(1.f, 1.f, 1.f, 1.f);
			glDisable(GL_BLEND);
		}

		NativeRenderer_EnableDepth(1);
		break;
	case BM_AVERAGE:
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		// NOTE(aalhendi): keep RGB blend weight constant so alpha can carry the PS1 mask bit.
		glBlendFuncSeparate(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA, GL_ONE, GL_ZERO);
		break;
	case BM_ADD:
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);
		break;
	case BM_SUBTRACT:
		glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_ADD);
		glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);
		break;
	case BM_ADD_QUATER_SOURCE:
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFuncSeparate(GL_CONSTANT_COLOR, GL_ONE, GL_ONE, GL_ZERO);
		break;
	}

	s_previousBlendMode = blendMode;
}

internal void NativeRenderer_SetViewPort(int x, int y, int width, int height)
{
	glViewport(x, y, width, height);
}

internal void NativeRenderer_SetWireframe(int enable)
{
	if (!s_rendererDialect.wireframeEnabled)
	{
		return;
	}
	glPolygonMode(GL_FRONT_AND_BACK, enable ? GL_LINE : GL_FILL);
}

void NativeRenderer_UpdateVertexBuffer(const GrVertex *vertices, int num_vertices)
{
	NativePerf_BeginScope(NATIVE_PERF_BUCKET_RENDERER_VERTEX_UPLOAD);
	if ((u32)num_vertices >= MAX_VERTEX_BUFFER_SIZE)
	{
		NATIVE_RENDERER_ERROR("%s\n", "MAX_VERTEX_BUFFER_SIZE reached, expect rendering errors");
		num_vertices = MAX_VERTEX_BUFFER_SIZE;
	}

	const int bufferIndex = s_curVertexBuffer;
	s_curVertexBuffer = (s_curVertexBuffer + 1) & 1;
	s_boundVertexBuffer = bufferIndex;
	glBindVertexArray(s_glVertexArray[bufferIndex]);
	glBindBuffer(GL_ARRAY_BUFFER, s_glVertexBuffer[bufferIndex]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, num_vertices * sizeof(GrVertex), vertices);
	NativePerf_EndScope(NATIVE_PERF_BUCKET_RENDERER_VERTEX_UPLOAD);
}

void NativeRenderer_DrawTriangles(int start_vertex, int triangles)
{
	NativePerf_BeginScope(NATIVE_PERF_BUCKET_RENDERER_DRAW_TRIANGLES);
	NativePerf_AddCounter(NATIVE_PERF_COUNTER_RENDERER_DRAW_CALLS, 1);
	NativePerf_AddCounter(NATIVE_PERF_COUNTER_RENDERER_DRAW_VERTICES, (u32)(triangles * 3));
	glDrawArrays(GL_TRIANGLES, start_vertex, triangles * 3);
	NativePerf_EndScope(NATIVE_PERF_BUCKET_RENDERER_DRAW_TRIANGLES);
}

void NativeRenderer_PushDebugLabel(const char *label)
{
	if (!s_rendererDialect.debugLabelsEnabled || !GLAD_GL_KHR_debug)
	{
		return;
	}
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0x8000, strlen(label), label);
}

void NativeRenderer_PopDebugLabel(void)
{
	if (!s_rendererDialect.debugLabelsEnabled || !GLAD_GL_KHR_debug)
	{
		return;
	}
	glPopDebugGroup();
}

int NativeRenderer_RunDialectSelfTest(void)
{
	// Platform cleanup is intentionally idempotent even when context creation
	// never reached the GL entry-point loader.
	NativeRenderer_Shutdown();
	NativeRenderer_Shutdown();

#if defined(CTR_NATIVE_RENDERER_GLES)
	const b32 valid = (s_rendererDialect.contextMajor == 3) && (s_rendererDialect.contextMinor == 0) &&
	                  (s_rendererDialect.contextProfile == SDL_GL_CONTEXT_PROFILE_ES) &&
	                  (strstr(s_rendererDialect.vertexShaderHeader, "#version 300 es") != NULL) &&
	                  (strstr(s_rendererDialect.fragmentShaderHeader, "precision mediump int") != NULL) &&
	                  !s_rendererDialect.wireframeEnabled && !s_rendererDialect.debugLabelsEnabled &&
	                  !s_rendererDialect.gpuTimerEnabled;
#else
	const b32 valid = (s_rendererDialect.contextMajor == 3) && (s_rendererDialect.contextMinor == 3) &&
	                  (s_rendererDialect.contextProfile == SDL_GL_CONTEXT_PROFILE_CORE) &&
	                  (strstr(s_rendererDialect.vertexShaderHeader, "#version 140") != NULL) &&
	                  (strstr(s_rendererDialect.fragmentShaderHeader, "#version 140") != NULL) &&
	                  s_rendererDialect.wireframeEnabled && s_rendererDialect.debugLabelsEnabled &&
	                  s_rendererDialect.gpuTimerEnabled;
#endif

	if (!valid)
	{
		fprintf(stderr, "[CTR Renderer] dialect self-test failed\n");
		return 1;
	}

	printf("[CTR Renderer] dialect self-test passed: api=%s context=%d.%d profile=%s shader=%s loader=%s "
	       "wireframe=%s debug-labels=%s gpu-timer=%s shutdown=preinit-safe\n",
	       s_rendererDialect.apiName, s_rendererDialect.contextMajor, s_rendererDialect.contextMinor,
	       s_rendererDialect.profileName, s_rendererDialect.shaderName, s_rendererDialect.loaderName,
	       s_rendererDialect.wireframeEnabled ? "enabled" : "disabled",
	       s_rendererDialect.debugLabelsEnabled ? "enabled" : "disabled",
	       s_rendererDialect.gpuTimerEnabled ? "enabled" : "disabled");
	return 0;
}

#define NATIVE_RENDERER_PIXEL_TEST_WIDTH  32
#define NATIVE_RENDERER_PIXEL_TEST_HEIGHT 16
#define NATIVE_RENDERER_PIXEL_TEST_PRESENT_SCALE 2
#define NATIVE_RENDERER_PIXEL_TEST_PRESENT_WIDTH (NATIVE_RENDERER_PIXEL_TEST_WIDTH * NATIVE_RENDERER_PIXEL_TEST_PRESENT_SCALE)
#define NATIVE_RENDERER_PIXEL_TEST_PRESENT_HEIGHT (NATIVE_RENDERER_PIXEL_TEST_HEIGHT * NATIVE_RENDERER_PIXEL_TEST_PRESENT_SCALE)

internal void NativeRenderer_PixelTestCopyRows(u16 *dst, int rowWords, int rows, const u16 *row)
{
	for (int y = 0; y < rows; y++)
	{
		memcpy(dst + y * rowWords, row, (size_t)rowWords * sizeof(*row));
	}
}

internal void NativeRenderer_PixelTestInitQuad(POLY_FT4 *quad, int x, int y, int width, int height, int format, int blend,
	                                             int pageX, int pageY, int clutX, int clutY, int semiTrans)
{
	memset(quad, 0, sizeof(*quad));
	setPolyFT4(quad);
	setRGB0(quad, 128, 128, 128);
	setXYWH(quad, x, y, width, height);
	setUVWH(quad, 0, 0, width, height);
	setTPage(quad, format, blend, pageX, pageY);
	setClut(quad, clutX, clutY);
	setSemiTrans(quad, semiTrans);
}

internal void NativeRenderer_PixelTestClearBlendTarget(void)
{
	NativeRenderer_BindMainRenderTarget();
	NativeRenderer_SetViewPort(0, 0, NATIVE_RENDERER_PIXEL_TEST_WIDTH, NATIVE_RENDERER_PIXEL_TEST_HEIGHT);
	NativeRenderer_SetBlendMode(BM_NONE);
	NativeRenderer_SetScissorState(0);
	glClearColor(0.0f, 0.0f, 248.0f / 255.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

internal int NativeRenderer_PixelTestDrawBlendFixture(void)
{
	POLY_FT4 blendQuads[4];
	POLY_FT4 orderedQuads[2];
	POLY_FT4 bilinearQuad;
	const int previousBilinearFiltering = g_cfg_bilinearFiltering;
	int firstPhaseDrawCalls;

	for (int blendMode = 0; blendMode < 4; blendMode++)
	{
		NativeRenderer_PixelTestInitQuad(&blendQuads[blendMode], blendMode * 4, 4, 4, 4, 0, blendMode, 256, 0, 0, 480, 1);
		ParsePrimitivesLinkedList((u32 *)&blendQuads[blendMode], 1);
	}
	for (int i = 0; i < 2; i++)
	{
		NativeRenderer_PixelTestInitQuad(&orderedQuads[i], 24, 4, 4, 4, 0, 0, 256, 0, 0, 480, 1);
		ParsePrimitivesLinkedList((u32 *)&orderedQuads[i], 1);
	}
	DrawAllSplits();
	firstPhaseDrawCalls = NativeGpu_GetLastRendererDrawCount();
	g_cfg_bilinearFiltering = 1;
	NativeRenderer_PixelTestInitQuad(&bilinearQuad, 20, 4, 4, 4, 0, 0, 256, 0, 0, 480, 1);
	ParsePrimitivesLinkedList((u32 *)&bilinearQuad, 1);
	DrawAllSplits();
	g_cfg_bilinearFiltering = previousBilinearFiltering;
	return firstPhaseDrawCalls;
}

internal int NativeRenderer_PixelTestExpectBufferMatch(const u8 *expected, const u8 *actual, size_t size, const char *label)
{
	if (memcmp(expected, actual, size) == 0)
	{
		return 1;
	}

	for (size_t i = 0; i < size; i++)
	{
		if (expected[i] != actual[i])
		{
			const size_t pixel = i / 4u;
			const int x = (int)(pixel % NATIVE_RENDERER_PIXEL_TEST_WIDTH);
			const int y = (int)(pixel / NATIVE_RENDERER_PIXEL_TEST_WIDTH);
			const int channel = (int)(i % 4u);
			fprintf(stderr, "[CTR Renderer] pixel self-test mismatch: %s at (%d,%d) channel=%d expected=%u actual=%u\n", label, x, y,
			        channel, expected[i], actual[i]);
			break;
		}
	}

	return 0;
}

internal int NativeRenderer_PixelTestCaptureMainRGBA(u8 *dst)
{
	u8 bottomUp[NATIVE_RENDERER_PIXEL_TEST_WIDTH * NATIVE_RENDERER_PIXEL_TEST_HEIGHT * 4];
	GLint previousFramebuffer = 0;
	GLint previousPackAlignment = 0;

	if ((s_mainRenderTarget.width != NATIVE_RENDERER_PIXEL_TEST_WIDTH) || (s_mainRenderTarget.height != NATIVE_RENDERER_PIXEL_TEST_HEIGHT))
	{
		return 0;
	}

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
	glGetIntegerv(GL_PACK_ALIGNMENT, &previousPackAlignment);
	glBindFramebuffer(GL_FRAMEBUFFER, s_mainRenderTarget.framebuffer);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, NATIVE_RENDERER_PIXEL_TEST_WIDTH, NATIVE_RENDERER_PIXEL_TEST_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, bottomUp);
	glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);
	glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)previousFramebuffer);

	for (int y = 0; y < NATIVE_RENDERER_PIXEL_TEST_HEIGHT; y++)
	{
		const int srcY = NATIVE_RENDERER_PIXEL_TEST_HEIGHT - y - 1;
		memcpy(dst + y * NATIVE_RENDERER_PIXEL_TEST_WIDTH * 4, bottomUp + srcY * NATIVE_RENDERER_PIXEL_TEST_WIDTH * 4,
		       NATIVE_RENDERER_PIXEL_TEST_WIDTH * 4);
	}

	return glGetError() == GL_NO_ERROR;
}

internal const u8 *NativeRenderer_PixelTestPixel(const u8 *rgba, int x, int y)
{
	return rgba + (y * NATIVE_RENDERER_PIXEL_TEST_WIDTH + x) * 4;
}

internal int NativeRenderer_PixelTestNear(u8 actual, int expected, int tolerance)
{
	const int delta = (int)actual - expected;
	return (delta >= -tolerance) && (delta <= tolerance);
}

internal int NativeRenderer_PixelTestExpectRGBA(const u8 *rgba, int x, int y, int r, int g, int b, int a, int tolerance,
	                                             const char *label)
{
	const u8 *pixel = NativeRenderer_PixelTestPixel(rgba, x, y);
	if (NativeRenderer_PixelTestNear(pixel[0], r, tolerance) && NativeRenderer_PixelTestNear(pixel[1], g, tolerance) &&
	    NativeRenderer_PixelTestNear(pixel[2], b, tolerance) && (pixel[3] == a))
	{
		return 1;
	}

	fprintf(stderr, "[CTR Renderer] pixel self-test mismatch: %s at (%d,%d) actual=%u,%u,%u,%u expected=%d,%d,%d,%d tolerance=%d\n",
	        label, x, y, pixel[0], pixel[1], pixel[2], pixel[3], r, g, b, a, tolerance);
	return 0;
}

internal int NativeRenderer_PixelTestExpectVRAM(const u16 *vram, int x, int y, u16 expected, const char *label)
{
	const u16 actual = vram[y * NATIVE_RENDERER_PIXEL_TEST_WIDTH + x];
	if (actual == expected)
	{
		return 1;
	}

	fprintf(stderr, "[CTR Renderer] pixel self-test VRAM mismatch: %s at (%d,%d) actual=%04x expected=%04x\n", label, x, y, actual,
	        expected);
	return 0;
}

internal u64 NativeRenderer_PixelTestHash(const u8 *data, size_t size)
{
	u64 hash = UINT64_C(14695981039346656037);
	for (size_t i = 0; i < size; i++)
	{
		hash ^= data[i];
		hash *= UINT64_C(1099511628211);
	}
	return hash;
}

int NativeRenderer_RunPixelSelfTest(void)
{
	const u16 transparent = 0x0000;
	const u16 red = 0x001f;
	const u16 greenStp = 0x83e0;
	const u16 blue = 0x7c00;
	const u16 whiteStp = 0xffff;
	const u16 texture4Row[1] = {0x3210};
	const u16 texture8Row[2] = {0x0201, 0x0300};
	const u16 texture16Row[4] = {red, greenStp, transparent, whiteStp};
	u16 texture4[4];
	u16 texture8[8];
	u16 texture16[16];
	u16 clut4[16];
	u16 clut8[256];
	u16 packed[NATIVE_RENDERER_PIXEL_TEST_WIDTH * NATIVE_RENDERER_PIXEL_TEST_HEIGHT];
	u8 rgba[NATIVE_RENDERER_PIXEL_TEST_WIDTH * NATIVE_RENDERER_PIXEL_TEST_HEIGHT * 4];
	u8 blendRgba[NATIVE_RENDERER_PIXEL_TEST_WIDTH * NATIVE_RENDERER_PIXEL_TEST_HEIGHT * 4];
	u8 blendFallbackRgba[NATIVE_RENDERER_PIXEL_TEST_WIDTH * NATIVE_RENDERER_PIXEL_TEST_HEIGHT * 4];
	u8 *presentDirect = NULL;
	u8 *presentStaged = NULL;
	POLY_FT4 quad4;
	POLY_FT4 quad4Semi;
	POLY_FT4 quad8;
	POLY_FT4 quad16;
	POLY_FT4 feedback;
	DR_STP mask;
	TILE maskTile;
	int passed = 1;
	int framebufferFetchUsed = 0;
	int blendFallbackDrawCalls;
	int blendActiveDrawCalls;
	int presentWidth;
	int presentHeight;
	size_t presentBytes;
	u64 hash;
	u64 presentHash;
	u64 blendHash;
	u64 blendOracleHash;
	b32 framebufferFetchAvailable;

	memset(rgba, 0, sizeof(rgba));
	memset(blendRgba, 0, sizeof(blendRgba));
	memset(blendFallbackRgba, 0, sizeof(blendFallbackRgba));
	memset(clut4, 0, sizeof(clut4));
	memset(clut8, 0, sizeof(clut8));
	clut4[1] = red;
	clut4[2] = greenStp;
	clut4[3] = blue;
	clut8[1] = red;
	clut8[2] = greenStp;
	clut8[3] = blue;
	NativeRenderer_PixelTestCopyRows(texture4, 1, 4, texture4Row);
	NativeRenderer_PixelTestCopyRows(texture8, 2, 4, texture8Row);
	NativeRenderer_PixelTestCopyRows(texture16, 4, 4, texture16Row);

	Platform_Init("CTR Renderer Pixel Self-Test", NATIVE_RENDERER_PIXEL_TEST_PRESENT_WIDTH, NATIVE_RENDERER_PIXEL_TEST_PRESENT_HEIGHT);
	if (!Platform_IsInitialized())
	{
		fprintf(stderr, "[CTR Renderer] pixel self-test failed: platform initialization\n");
		return 1;
	}
	// The pixel oracle owns a fixed logical baseline. Ignore any persisted host
	// presentation scale so a user's 2x-4x preference cannot resize its target.
	NativeRenderer_SetInternalResolutionScale(1);
	presentWidth = g_windowWidth;
	presentHeight = g_windowHeight;
	if ((presentWidth <= 0) || (presentHeight <= 0) ||
	    ((size_t)presentWidth > (SIZE_MAX / 4u) / (size_t)presentHeight))
	{
		fprintf(stderr, "[CTR Renderer] pixel self-test failed: invalid presentation size %dx%d\n", presentWidth, presentHeight);
		Platform_Shutdown();
		return 1;
	}
	presentBytes = (size_t)presentWidth * (size_t)presentHeight * 4u;
	presentDirect = (u8 *)SDL_malloc(presentBytes);
	presentStaged = (u8 *)SDL_malloc(presentBytes);
	if ((presentDirect == NULL) || (presentStaged == NULL))
	{
		fprintf(stderr, "[CTR Renderer] pixel self-test failed: presentation allocation (%zu bytes each)\n", presentBytes);
		SDL_free(presentDirect);
		SDL_free(presentStaged);
		Platform_Shutdown();
		return 1;
	}
	memset(presentDirect, 0, presentBytes);
	memset(presentStaged, 0, presentBytes);

	memset(&activeDispEnv, 0, sizeof(activeDispEnv));
	memset(&activeDrawEnv, 0, sizeof(activeDrawEnv));
	SetDefDispEnv(&activeDispEnv, 0, 0, NATIVE_RENDERER_PIXEL_TEST_WIDTH, NATIVE_RENDERER_PIXEL_TEST_HEIGHT);
	SetDefDrawEnv(&activeDrawEnv, 0, 0, NATIVE_RENDERER_PIXEL_TEST_WIDTH, NATIVE_RENDERER_PIXEL_TEST_HEIGHT);
	activeDrawEnv.dfe = 1;
	activeDrawEnv.dtd = 0;
	activeDrawEnv.isbg = 1;
	activeDrawEnv.r0 = 0;
	activeDrawEnv.g0 = 0;
	activeDrawEnv.b0 = 248;
	ClearSplits();

	NativeRenderer_CopyVRAM(texture4, 0, 0, 1, 4, 256, 0);
	NativeRenderer_CopyVRAM(clut4, 0, 0, 16, 1, 0, 480);
	NativeRenderer_CopyVRAM(texture8, 0, 0, 2, 4, 320, 0);
	NativeRenderer_CopyVRAM(clut8, 0, 0, 256, 1, 256, 480);
	NativeRenderer_CopyVRAM(texture16, 0, 0, 4, 4, 512, 0);

	if (!Platform_BeginScene())
	{
		fprintf(stderr, "[CTR Renderer] pixel self-test failed: scene initialization\n");
		SDL_free(presentDirect);
		SDL_free(presentStaged);
		Platform_Shutdown();
		return 1;
	}

	NativeRenderer_PixelTestInitQuad(&quad4, 0, 0, 4, 4, 0, 0, 256, 0, 0, 480, 0);
	NativeRenderer_PixelTestInitQuad(&quad4Semi, 4, 0, 4, 4, 0, 0, 256, 0, 0, 480, 1);
	NativeRenderer_PixelTestInitQuad(&quad8, 8, 0, 4, 4, 1, 0, 320, 0, 256, 480, 0);
	NativeRenderer_PixelTestInitQuad(&quad16, 12, 0, 4, 4, 2, 0, 512, 0, 0, 0, 0);
	ParsePrimitivesLinkedList((u32 *)&quad4, 1);
	ParsePrimitivesLinkedList((u32 *)&quad4Semi, 1);
	ParsePrimitivesLinkedList((u32 *)&quad8, 1);
	ParsePrimitivesLinkedList((u32 *)&quad16, 1);

	memset(&mask, 0, sizeof(mask));
	setDrawStp(&mask, 1);
	ParsePrimitivesLinkedList((u32 *)&mask, 1);
	memset(&maskTile, 0, sizeof(maskTile));
	setTile(&maskTile);
	setRGB0(&maskTile, 255, 0, 0);
	setXY0(&maskTile, 16, 0);
	setWH(&maskTile, 4, 4);
	ParsePrimitivesLinkedList((u32 *)&maskTile, 1);
	DrawAllSplits();

	memset(&mask, 0, sizeof(mask));
	setDrawStp(&mask, 0);
	ParsePrimitivesLinkedList((u32 *)&mask, 1);
	NativeRenderer_PixelTestInitQuad(&feedback, 24, 0, 4, 4, 2, 0, 0, 0, 0, 0, 0);
	ParsePrimitivesLinkedList((u32 *)&feedback, 1);
	DrawAllSplits();

	if (!NativeRenderer_PixelTestCaptureMainRGBA(rgba))
	{
		fprintf(stderr, "[CTR Renderer] pixel self-test failed: RGBA readback\n");
		passed = 0;
	}

	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 0, 1, 0, 0, 248, 0, 2, "4-bit transparent index");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 1, 1, 248, 0, 0, 0, 2, "4-bit CLUT red");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 2, 1, 0, 248, 0, 255, 2, "4-bit CLUT STP green");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 3, 1, 0, 0, 248, 0, 2, "4-bit CLUT blue");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 4, 1, 0, 0, 248, 0, 2, "semi-transparent zero discard");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 5, 1, 248, 0, 0, 0, 2, "semi-transparent non-STP opaque pass");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 6, 1, 0, 124, 124, 255, 4, "semi-transparent STP average pass");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 8, 1, 248, 0, 0, 0, 2, "8-bit CLUT red");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 9, 1, 0, 248, 0, 255, 2, "8-bit CLUT STP green");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 10, 1, 0, 0, 248, 0, 2, "8-bit transparent index");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 11, 1, 0, 0, 248, 0, 2, "8-bit CLUT blue");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 12, 1, 248, 0, 0, 0, 2, "16-bit direct red");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 13, 1, 0, 248, 0, 255, 2, "16-bit direct STP green");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 14, 1, 0, 0, 248, 0, 2, "16-bit transparent zero");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 15, 1, 248, 248, 248, 255, 2, "16-bit direct STP white");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 16, 1, 248, 0, 0, 255, 2, "forced output mask bit");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 24, 1, 0, 0, 248, 0, 2, "framebuffer feedback blue");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 25, 1, 248, 0, 0, 0, 2, "framebuffer feedback red");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 26, 1, 0, 248, 0, 255, 2, "framebuffer feedback STP green");
	passed &= NativeRenderer_PixelTestExpectRGBA(rgba, 27, 1, 0, 0, 248, 0, 2, "framebuffer feedback blue copy");

	NativeRenderer_StoreFrameBuffer(0, 0, NATIVE_RENDERER_PIXEL_TEST_WIDTH, NATIVE_RENDERER_PIXEL_TEST_HEIGHT);
	NativeRenderer_ReadVRAM(packed, 0, 0, NATIVE_RENDERER_PIXEL_TEST_WIDTH, NATIVE_RENDERER_PIXEL_TEST_HEIGHT);
	passed &= NativeRenderer_PixelTestExpectVRAM(packed, 0, 1, blue, "transparent index preserves background");
	passed &= NativeRenderer_PixelTestExpectVRAM(packed, 1, 1, red, "red mask clear");
	passed &= NativeRenderer_PixelTestExpectVRAM(packed, 2, 1, greenStp, "sampled STP mask set");
	passed &= NativeRenderer_PixelTestExpectVRAM(packed, 16, 1, (u16)(0x8000 | red), "forced mask set");
	passed &= NativeRenderer_PixelTestExpectVRAM(packed, 24, 1, blue, "feedback packed blue");
	passed &= NativeRenderer_PixelTestExpectVRAM(packed, 25, 1, red, "feedback packed red");
	passed &= NativeRenderer_PixelTestExpectVRAM(packed, 26, 1, greenStp, "feedback packed STP green");

	NativeRenderer_PresentVRAMRectDirect(0, 0, NATIVE_RENDERER_PIXEL_TEST_WIDTH, NATIVE_RENDERER_PIXEL_TEST_HEIGHT);
	if (!NativeRenderer_CapturePresentedRGBA(presentDirect, presentWidth, presentHeight))
	{
		fprintf(stderr, "[CTR Renderer] pixel self-test failed: direct presentation readback\n");
		passed = 0;
	}
	NativeRenderer_PresentVRAMRect(0, 0, NATIVE_RENDERER_PIXEL_TEST_WIDTH, NATIVE_RENDERER_PIXEL_TEST_HEIGHT);
	if (!NativeRenderer_CapturePresentedRGBA(presentStaged, presentWidth, presentHeight))
	{
		fprintf(stderr, "[CTR Renderer] pixel self-test failed: staged presentation readback\n");
		passed = 0;
	}
	if (memcmp(presentDirect, presentStaged, presentBytes) != 0)
	{
		fprintf(stderr, "[CTR Renderer] pixel self-test failed: staged presentation differs from direct oracle\n");
		passed = 0;
	}

	hash = NativeRenderer_PixelTestHash(rgba, sizeof(rgba));
	presentHash = NativeRenderer_PixelTestHash(presentStaged, presentBytes);

	// Exercise every PS1 semitransparency equation over the same blue
	// destination. On a framebuffer-fetch implementation, render the identical
	// fixture through the portable two-pass oracle first and require every byte
	// of the optimized draw to match it.
	framebufferFetchAvailable = s_psxFramebufferFetchEnabled;
	s_psxFramebufferFetchEnabled = false;
	NativeRenderer_PixelTestClearBlendTarget();
	blendFallbackDrawCalls = NativeRenderer_PixelTestDrawBlendFixture();
	if (blendFallbackDrawCalls != 12)
	{
		fprintf(stderr, "[CTR Renderer] pixel self-test failed: two-pass ordered-overlap draws expected=12 actual=%d\n", blendFallbackDrawCalls);
		passed = 0;
	}
	if (!NativeRenderer_PixelTestCaptureMainRGBA(blendFallbackRgba))
	{
		fprintf(stderr, "[CTR Renderer] pixel self-test failed: two-pass blend oracle readback\n");
		passed = 0;
	}

	if (framebufferFetchAvailable)
	{
		s_psxFramebufferFetchEnabled = true;
		NativeRenderer_PixelTestClearBlendTarget();
		blendActiveDrawCalls = NativeRenderer_PixelTestDrawBlendFixture();
		framebufferFetchUsed = NativeRenderer_UsesFramebufferFetch();
		if (blendActiveDrawCalls != 5)
		{
			fprintf(stderr, "[CTR Renderer] pixel self-test failed: framebuffer-fetch ordered-overlap draws expected=5 actual=%d\n", blendActiveDrawCalls);
			passed = 0;
		}
		if (!NativeRenderer_PixelTestCaptureMainRGBA(blendRgba))
		{
			fprintf(stderr, "[CTR Renderer] pixel self-test failed: framebuffer-fetch blend readback\n");
			passed = 0;
		}
		passed &= NativeRenderer_PixelTestExpectBufferMatch(blendFallbackRgba, blendRgba, sizeof(blendRgba),
		                                                      "framebuffer fetch differs from two-pass oracle");
	}
	else
	{
		blendActiveDrawCalls = blendFallbackDrawCalls;
		memcpy(blendRgba, blendFallbackRgba, sizeof(blendRgba));
	}
	s_psxFramebufferFetchEnabled = framebufferFetchAvailable;
	if (!NativeRenderer_PixelTestCaptureMainRGBA(blendRgba))
	{
		fprintf(stderr, "[CTR Renderer] pixel self-test failed: final blend-mode RGBA readback\n");
		passed = 0;
	}
	passed &= NativeRenderer_PixelTestExpectRGBA(blendRgba, 1, 5, 248, 0, 0, 0, 2, "average non-STP opaque");
	passed &= NativeRenderer_PixelTestExpectRGBA(blendRgba, 2, 5, 0, 124, 124, 255, 4, "average STP blend");
	passed &= NativeRenderer_PixelTestExpectRGBA(blendRgba, 5, 5, 248, 0, 0, 0, 2, "add non-STP opaque");
	passed &= NativeRenderer_PixelTestExpectRGBA(blendRgba, 6, 5, 0, 248, 248, 255, 4, "add STP blend");
	passed &= NativeRenderer_PixelTestExpectRGBA(blendRgba, 9, 5, 248, 0, 0, 0, 2, "subtract non-STP opaque");
	passed &= NativeRenderer_PixelTestExpectRGBA(blendRgba, 10, 5, 0, 0, 248, 255, 4, "subtract STP blend");
	passed &= NativeRenderer_PixelTestExpectRGBA(blendRgba, 13, 5, 248, 0, 0, 0, 2, "quarter-add non-STP opaque");
	passed &= NativeRenderer_PixelTestExpectRGBA(blendRgba, 14, 5, 0, 62, 248, 255, 4, "quarter-add STP blend");
	passed &= NativeRenderer_PixelTestExpectRGBA(blendRgba, 21, 5, 124, 124, 0, 255, 4, "bilinear mixed STP/non-STP");
	passed &= NativeRenderer_PixelTestExpectRGBA(blendRgba, 25, 5, 248, 0, 0, 0, 2, "ordered overlap non-STP opaque");
	passed &= NativeRenderer_PixelTestExpectRGBA(blendRgba, 26, 5, 0, 186, 62, 255, 4, "ordered overlap STP twice blended");
	blendHash = NativeRenderer_PixelTestHash(blendRgba, sizeof(blendRgba));
	blendOracleHash = NativeRenderer_PixelTestHash(blendFallbackRgba, sizeof(blendFallbackRgba));

	// Verify the enhanced path independently of presentation screenshots: the
	// main target scales, logical clears cover the matching physical pixels,
	// framebuffer feedback still lands in logical PS1 VRAM, and presentation
	// reads from the high-resolution target.
	for (int scale = 2; scale <= 4; scale++)
	{
		if (NativeRenderer_SetInternalResolutionScale(scale) != scale)
		{
			fprintf(stderr, "[CTR Renderer] pixel self-test failed: %dx internal resolution unavailable\n", scale);
			passed = 0;
		}
		NativeRenderer_BindMainRenderTarget();
		if ((s_mainRenderTarget.width != NATIVE_RENDERER_PIXEL_TEST_WIDTH * scale) ||
		    (s_mainRenderTarget.height != NATIVE_RENDERER_PIXEL_TEST_HEIGHT * scale))
		{
			fprintf(stderr, "[CTR Renderer] pixel self-test failed: %dx target expected=%dx%d actual=%dx%d\n", scale,
			        NATIVE_RENDERER_PIXEL_TEST_WIDTH * scale, NATIVE_RENDERER_PIXEL_TEST_HEIGHT * scale,
			        s_mainRenderTarget.width, s_mainRenderTarget.height);
			passed = 0;
		}
	}
	NativeRenderer_SetInternalResolutionScale(2);
	NativeRenderer_BindMainRenderTarget();
	NativeRenderer_SetViewPort(0, 0, s_mainRenderTarget.width, s_mainRenderTarget.height);
	NativeRenderer_SetBlendMode(BM_NONE);
	NativeRenderer_SetScissorState(0);
	glClearColor(0.0f, 0.0f, 248.0f / 255.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	NativeRenderer_Clear(0, 0, NATIVE_RENDERER_PIXEL_TEST_WIDTH / 2, NATIVE_RENDERER_PIXEL_TEST_HEIGHT, 255, 0, 0);
	NativeRenderer_StoreFrameBuffer(0, 0, NATIVE_RENDERER_PIXEL_TEST_WIDTH, NATIVE_RENDERER_PIXEL_TEST_HEIGHT);
	NativeRenderer_ReadVRAM(packed, 0, 0, NATIVE_RENDERER_PIXEL_TEST_WIDTH, NATIVE_RENDERER_PIXEL_TEST_HEIGHT);
	passed &= NativeRenderer_PixelTestExpectVRAM(packed, 8, 8, red, "2x scaled clear packs red");
	passed &= NativeRenderer_PixelTestExpectVRAM(packed, 24, 8, blue, "2x scaled clear preserves blue");
	NativeRenderer_PresentMainRenderTarget();
	if (!NativeRenderer_CapturePresentedRGBA(presentStaged, presentWidth, presentHeight))
	{
		fprintf(stderr, "[CTR Renderer] pixel self-test failed: 2x presentation readback\n");
		passed = 0;
	}
	else
	{
		const u8 *leftPixel = presentStaged + ((presentHeight / 2) * presentWidth + presentWidth / 4) * 4;
		const u8 *rightPixel = presentStaged + ((presentHeight / 2) * presentWidth + (presentWidth * 3) / 4) * 4;
		if (!NativeRenderer_PixelTestNear(leftPixel[0], 248, 2) || !NativeRenderer_PixelTestNear(leftPixel[1], 0, 2) ||
		    !NativeRenderer_PixelTestNear(leftPixel[2], 0, 2) || !NativeRenderer_PixelTestNear(rightPixel[0], 0, 2) ||
		    !NativeRenderer_PixelTestNear(rightPixel[1], 0, 2) || !NativeRenderer_PixelTestNear(rightPixel[2], 248, 2))
		{
			fprintf(stderr, "[CTR Renderer] pixel self-test failed: 2x direct presentation colors\n");
			passed = 0;
		}
	}
	NativeRenderer_SetInternalResolutionScale(1);
	SDL_free(presentDirect);
	SDL_free(presentStaged);
	Platform_EndScene();
	Platform_LogFlush();
	Platform_Shutdown();

	if (!passed)
	{
		fprintf(stderr, "[CTR Renderer] pixel self-test failed: api=%s hash=%016llx\n", s_rendererDialect.apiName,
		        (unsigned long long)hash);
		return 1;
	}

	printf("[CTR Renderer] pixel self-test passed: api=%s size=32x16 formats=4,8,16 clut=4,8 transparency=zero,stp "
	       "blend=average,add,subtract,quarter bilinear=mixed-stp ordered-overlap=match fallback-draws=%d active-draws=%d "
	       "mask=output-bit framebuffer=feedback vram=rgb5551 hash=%016llx "
	       "blend-hash=%016llx blend-oracle=%s oracle-hash=%016llx framebuffer-fetch=%s present=resolve+blit@%dx%d "
	       "present-hash=%016llx internal-scale=2x,3x,4x\n",
	       s_rendererDialect.apiName, blendFallbackDrawCalls, blendActiveDrawCalls, (unsigned long long)hash, (unsigned long long)blendHash,
	       framebufferFetchUsed ? "match" : "two-pass", (unsigned long long)blendOracleHash,
	       framebufferFetchUsed ? "enabled" : "two-pass", presentWidth, presentHeight, (unsigned long long)presentHash);
	fflush(stdout);
	return 0;
}

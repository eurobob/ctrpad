#ifndef NATIVE_VISION_H
#define NATIVE_VISION_H

#include <stddef.h>
#include <stdint.h>

struct GameTracker;
struct PushBuffer;

#ifdef __cplusplus
extern "C" {
#endif

enum NativeVisionMode
{
	NATIVE_VISION_MODE_WINDOW = 0,
	NATIVE_VISION_MODE_PORTAL = 1,
	NATIVE_VISION_MODE_COCKPIT = 2,
};

enum NativeVisionRenderLayer
{
	NATIVE_VISION_LAYER_LEFT = 0,
	NATIVE_VISION_LAYER_RIGHT = 1,
	NATIVE_VISION_LAYER_HUD = 2,
	NATIVE_VISION_LAYER_COUNT = 3,
};

/*
 * Compositor-to-game tracking ABI. Positions use the reference-head basis:
 * +X right, +Y up, +Z back. Rotations are relative pitch/yaw/roll in radians.
 */
struct NativeVisionTrackingFrame
{
	uint64_t timestampNanoseconds;
	float eyePositionMetres[2][3];
	float eyeRotationRadians[2][3];
	float convergenceMetres;
	float unitsPerMetre;
	uint32_t valid;
	uint32_t reserved;
};

void NativeVision_SetMode(int mode);
int NativeVision_GetMode(void);
void NativeVision_PublishTrackingFrame(const struct NativeVisionTrackingFrame *frame);
void NativeVision_PublishEyeTracking(uint64_t timestampNanoseconds,
	float leftX, float leftY, float leftZ, float leftPitch, float leftYaw, float leftRoll,
	float rightX, float rightY, float rightZ, float rightPitch, float rightYaw, float rightRoll,
	float convergenceMetres, float unitsPerMetre);
void NativeVision_ResetTracking(void);
float NativeVision_SetStereoDepthScale(float scale);

/* Returns 1 for the supported raw NTSC-U image, otherwise 0 with a message. */
int NativeVision_ValidateDiscImage(const char *path, char *message, size_t messageSize);

int NativeVision_IsStereoActive(void);
int NativeVision_GetRenderLayerCount(void);
int NativeVision_GetAllocationLayerCount(void);
int NativeVision_GetPrimitiveMemoryScale(void);

void NativeVision_UpdateCockpitCameraMode(struct GameTracker *gGT);
int NativeVision_BeginStereoFrame(struct GameTracker *gGT);
void NativeVision_BeginEye(struct GameTracker *gGT, int eyeIndex);
void NativeVision_BeginHud(struct GameTracker *gGT);
void NativeVision_EndStereoFrame(struct GameTracker *gGT);
void NativeVision_AddLayerMarker(struct GameTracker *gGT, int layer);
void NativeVision_AdjustGeomOffset(const struct PushBuffer *pb, int *x, int *y);
int NativeVision_IsCockpitPass(void);
int NativeVision_ShouldAdvanceRenderState(void);

int NativeVision_RunStereoMathSelfTest(void);
int NativeVision_RunMainStepWithAutoreleasePool(void);

#ifdef __cplusplus
}
#endif

#endif

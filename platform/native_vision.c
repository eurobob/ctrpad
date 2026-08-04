#include <common.h>

#include <platform/native_assets.h>
#include <platform/native_vision.h>
#include <platform/native_disc_image.h>

#include <math.h>
#include <stdio.h>
#include <stdatomic.h>
#include <string.h>

#define NATIVE_VISION_DEFAULT_IPD_METRES         0.064f
#define NATIVE_VISION_DEFAULT_CONVERGENCE_METRES 4.0f
#define NATIVE_VISION_DEFAULT_UNITS_PER_METRE    256.0f
#define NATIVE_VISION_RADIANS_TO_ANGLE           (4096.0f / 6.28318530717958647692f)

struct NativeVisionLayerPacket
{
	u32 tag;
	u8 pad0;
	u8 pad1;
	u8 pad2;
	u8 code;
	u32 layer;
};

CTR_STATIC_ASSERT(sizeof(struct NativeVisionLayerPacket) == 12);

struct NativeVisionRuntime
{
	atomic_flag trackingLock;
	_Atomic int mode;
	struct NativeVisionTrackingFrame tracking;
	struct NativeVisionTrackingFrame frameTracking;
	struct PushBuffer savedPushBuffer;
	u32 *layerOT[NATIVE_VISION_LAYER_COUNT];
	u32 *savedUiOT;
	u32 *savedOtMemUiOT;
	u32 savedDriverFlags;
	int savedCameraMode;
	int cameraModeOverridden;
	int currentEye;
	int currentLayer;
	int frameMode;
	int driverHidden;
	int frameActive;
};

static struct NativeVisionRuntime s_nativeVision = {
	.trackingLock = ATOMIC_FLAG_INIT,
	.mode = NATIVE_VISION_MODE_WINDOW,
	.savedCameraMode = -1,
	.currentEye = -1,
	.currentLayer = NATIVE_VISION_LAYER_HUD,
	.frameMode = NATIVE_VISION_MODE_WINDOW,
};

static void NativeVision_LockTracking(void)
{
	while (atomic_flag_test_and_set_explicit(&s_nativeVision.trackingLock, memory_order_acquire))
	{
	}
}

static void NativeVision_UnlockTracking(void)
{
	atomic_flag_clear_explicit(&s_nativeVision.trackingLock, memory_order_release);
}

static struct NativeVisionTrackingFrame NativeVision_DefaultTracking(void)
{
	struct NativeVisionTrackingFrame frame;
	memset(&frame, 0, sizeof(frame));
	frame.eyePositionMetres[0][0] = NATIVE_VISION_DEFAULT_IPD_METRES * -0.5f;
	frame.eyePositionMetres[1][0] = NATIVE_VISION_DEFAULT_IPD_METRES * 0.5f;
	frame.convergenceMetres = NATIVE_VISION_DEFAULT_CONVERGENCE_METRES;
	frame.unitsPerMetre = NATIVE_VISION_DEFAULT_UNITS_PER_METRE;
	frame.valid = 1;
	return frame;
}

static struct NativeVisionTrackingFrame NativeVision_ReadTracking(void)
{
	struct NativeVisionTrackingFrame frame;
	NativeVision_LockTracking();
	frame = s_nativeVision.tracking;
	NativeVision_UnlockTracking();

	if (!frame.valid || !isfinite(frame.convergenceMetres) || (frame.convergenceMetres < 0.05f) ||
	    !isfinite(frame.unitsPerMetre) || (frame.unitsPerMetre < 1.0f))
	{
		frame = NativeVision_DefaultTracking();
	}
	return frame;
}

void NativeVision_SetMode(int mode)
{
	if ((mode < NATIVE_VISION_MODE_WINDOW) || (mode > NATIVE_VISION_MODE_COCKPIT))
	{
		mode = NATIVE_VISION_MODE_WINDOW;
	}
	atomic_store_explicit(&s_nativeVision.mode, mode, memory_order_release);
}

int NativeVision_GetMode(void)
{
	return atomic_load_explicit(&s_nativeVision.mode, memory_order_acquire);
}

void NativeVision_PublishTrackingFrame(const struct NativeVisionTrackingFrame *frame)
{
	if (frame == NULL)
	{
		return;
	}
	NativeVision_LockTracking();
	s_nativeVision.tracking = *frame;
	NativeVision_UnlockTracking();
}

void NativeVision_PublishEyeTracking(uint64_t timestampNanoseconds,
	float leftX, float leftY, float leftZ, float leftPitch, float leftYaw, float leftRoll,
	float rightX, float rightY, float rightZ, float rightPitch, float rightYaw, float rightRoll,
	float convergenceMetres, float unitsPerMetre)
{
	struct NativeVisionTrackingFrame frame;
	memset(&frame, 0, sizeof(frame));
	frame.timestampNanoseconds = timestampNanoseconds;
	frame.eyePositionMetres[0][0] = leftX;
	frame.eyePositionMetres[0][1] = leftY;
	frame.eyePositionMetres[0][2] = leftZ;
	frame.eyeRotationRadians[0][0] = leftPitch;
	frame.eyeRotationRadians[0][1] = leftYaw;
	frame.eyeRotationRadians[0][2] = leftRoll;
	frame.eyePositionMetres[1][0] = rightX;
	frame.eyePositionMetres[1][1] = rightY;
	frame.eyePositionMetres[1][2] = rightZ;
	frame.eyeRotationRadians[1][0] = rightPitch;
	frame.eyeRotationRadians[1][1] = rightYaw;
	frame.eyeRotationRadians[1][2] = rightRoll;
	frame.convergenceMetres = convergenceMetres;
	frame.unitsPerMetre = unitsPerMetre;
	frame.valid = 1;
	NativeVision_PublishTrackingFrame(&frame);
}

void NativeVision_ResetTracking(void)
{
	struct NativeVisionTrackingFrame frame = NativeVision_DefaultTracking();
	NativeVision_PublishTrackingFrame(&frame);
}

int NativeVision_ValidateDiscImage(const char *path, char *message, size_t messageSize)
{
	int valid = 0;

	if ((message != NULL) && (messageSize != 0))
	{
		message[0] = '\0';
	}
	if ((path == NULL) || (path[0] == '\0'))
	{
		if ((message != NULL) && (messageSize != 0))
		{
			snprintf(message, messageSize, "No disc image was selected.");
		}
		goto DONE;
	}
	if (!NativeAssets_InitWithDiscImage(".", NULL, path))
	{
		if ((message != NULL) && (messageSize != 0))
		{
			snprintf(message, messageSize,
			         "That file is not a readable single-track MODE2/2352 image. Select the .img data file, not .ccd or .sub.");
		}
		goto DONE;
	}
	if (!NativeDiscImage_IsExpectedNTSCU())
	{
		if ((message != NULL) && (messageSize != 0))
		{
			snprintf(message, messageSize,
			         "This disc identifies as %s. CTRPad currently requires the North American NTSC-U disc %s.",
			         NativeDiscImage_GetDiscID(), NativeDiscImage_GetExpectedDiscID());
		}
		goto DONE;
	}
	if (!NativeAssets_Validate())
	{
		if ((message != NULL) && (messageSize != 0))
		{
			snprintf(message, messageSize,
			         "This NTSC-U image opened, but required CTR files were missing or unreadable. Try a complete raw CloneCD .img data file.");
		}
		goto DONE;
	}

	valid = 1;

DONE:
	NativeDiscImage_Shutdown();
	return valid;
}

int NativeVision_IsStereoActive(void)
{
#if defined(SDL_PLATFORM_VISIONOS)
	return NativeVision_GetMode() != NATIVE_VISION_MODE_WINDOW;
#else
	return 0;
#endif
}

int NativeVision_GetRenderLayerCount(void)
{
	return NativeVision_IsStereoActive() ? NATIVE_VISION_LAYER_COUNT : 1;
}

int NativeVision_GetAllocationLayerCount(void)
{
#if defined(SDL_PLATFORM_VISIONOS)
	return NATIVE_VISION_LAYER_COUNT;
#else
	return 1;
#endif
}

int NativeVision_GetPrimitiveMemoryScale(void)
{
#if defined(SDL_PLATFORM_VISIONOS)
	/* Two complete world passes plus one HUD pass and their layer packets. */
	return 3;
#else
	return 1;
#endif
}

void NativeVision_UpdateCockpitCameraMode(struct GameTracker *gGT)
{
	if ((gGT == NULL) || (gGT->numPlyrCurrGame != 1))
	{
		return;
	}

	struct CameraDC *camera = &gGT->cameraDC[0];
	if (NativeVision_GetMode() == NATIVE_VISION_MODE_COCKPIT)
	{
		/* Track camera modes selected by level transitions while VR is active. */
		if (camera->cameraMode != 0xf)
		{
			s_nativeVision.savedCameraMode = camera->cameraMode;
		}
		s_nativeVision.cameraModeOverridden = 1;
		camera->cameraMode = 0xf;
	}
	else if (s_nativeVision.cameraModeOverridden)
	{
		camera->cameraMode = s_nativeVision.savedCameraMode >= 0 ? s_nativeVision.savedCameraMode : 0;
		s_nativeVision.savedCameraMode = -1;
		s_nativeVision.cameraModeOverridden = 0;
	}
}

static s16 NativeVision_ClampS16(float value)
{
	if (value < -32768.0f)
	{
		return -32768;
	}
	if (value > 32767.0f)
	{
		return 32767;
	}
	return (s16)lrintf(value);
}

static void NativeVision_ApplyEyePose(struct PushBuffer *pb, int eyeIndex)
{
	const struct NativeVisionTrackingFrame *tracking = &s_nativeVision.frameTracking;
	const float units = tracking->unitsPerMetre;
	MATRIX cameraToWorld;
	SVec3 baseRotation = pb->rot;
	ConvertRotToMatrix(&cameraToWorld, &baseRotation);

	float local[3] = {
	    tracking->eyePositionMetres[eyeIndex][0] * units,
	    tracking->eyePositionMetres[eyeIndex][1] * units,
	    tracking->eyePositionMetres[eyeIndex][2] * units,
	};
	float world[3];
	for (int row = 0; row < 3; row++)
	{
		world[row] = ((float)cameraToWorld.m[row][0] * local[0] + (float)cameraToWorld.m[row][1] * local[1] +
		              (float)cameraToWorld.m[row][2] * local[2]) /
		             4096.0f;
	}

	pb->pos.x = NativeVision_ClampS16((float)pb->pos.x + world[0]);
	pb->pos.y = NativeVision_ClampS16((float)pb->pos.y + world[1]);
	pb->pos.z = NativeVision_ClampS16((float)pb->pos.z + world[2]);

	/* Swift publishes pitch, yaw, roll. CTR uses one turn == 0x1000. */
	pb->rot.x = (s16)(pb->rot.x + lrintf(tracking->eyeRotationRadians[eyeIndex][0] * NATIVE_VISION_RADIANS_TO_ANGLE));
	pb->rot.y = (s16)(pb->rot.y + lrintf(tracking->eyeRotationRadians[eyeIndex][1] * NATIVE_VISION_RADIANS_TO_ANGLE));
	pb->rot.z = (s16)(pb->rot.z + lrintf(tracking->eyeRotationRadians[eyeIndex][2] * NATIVE_VISION_RADIANS_TO_ANGLE));
}

int NativeVision_BeginStereoFrame(struct GameTracker *gGT)
{
	int mode = NativeVision_GetMode();
	if ((gGT == NULL) || (mode == NATIVE_VISION_MODE_WINDOW))
	{
		return 0;
	}

	s_nativeVision.savedPushBuffer = gGT->pushBuffer[0];
	s_nativeVision.layerOT[NATIVE_VISION_LAYER_LEFT] = gGT->pushBuffer[0].ptrOT;
	s_nativeVision.layerOT[NATIVE_VISION_LAYER_RIGHT] = gGT->pushBuffer[1].ptrOT;
	s_nativeVision.layerOT[NATIVE_VISION_LAYER_HUD] = gGT->pushBuffer[2].ptrOT;
	s_nativeVision.savedUiOT = gGT->pushBuffer_UI.ptrOT;
	s_nativeVision.savedOtMemUiOT = gGT->backBuffer->otMem.uiOT;
	s_nativeVision.frameTracking = NativeVision_ReadTracking();
	s_nativeVision.currentEye = -1;
	s_nativeVision.frameMode = mode;
	s_nativeVision.frameActive = 1;
	return 1;
}

void NativeVision_BeginEye(struct GameTracker *gGT, int eyeIndex)
{
	if ((gGT == NULL) || !s_nativeVision.frameActive || (eyeIndex < 0) || (eyeIndex > 1))
	{
		return;
	}

	if (s_nativeVision.driverHidden && (gGT->drivers[0] != NULL) && (gGT->drivers[0]->instSelf != NULL))
	{
		gGT->drivers[0]->instSelf->flags = s_nativeVision.savedDriverFlags;
		s_nativeVision.driverHidden = 0;
	}

	struct PushBuffer *pb = &gGT->pushBuffer[0];
	*pb = s_nativeVision.savedPushBuffer;
	pb->ptrOT = s_nativeVision.layerOT[eyeIndex];
	gGT->pushBuffer_UI.ptrOT = pb->ptrOT;
	gGT->backBuffer->otMem.uiOT = pb->ptrOT;
	s_nativeVision.currentEye = eyeIndex;
	s_nativeVision.currentLayer = eyeIndex;

	NativeVision_ApplyEyePose(pb, eyeIndex);
	PushBuffer_UpdateFrustum(pb);

	if ((s_nativeVision.frameMode == NATIVE_VISION_MODE_COCKPIT) && (gGT->drivers[0] != NULL) &&
	    (gGT->drivers[0]->instSelf != NULL))
	{
		s_nativeVision.savedDriverFlags = gGT->drivers[0]->instSelf->flags;
		gGT->drivers[0]->instSelf->flags |= HIDE_MODEL;
		s_nativeVision.driverHidden = 1;
	}
}

void NativeVision_BeginHud(struct GameTracker *gGT)
{
	if ((gGT == NULL) || !s_nativeVision.frameActive)
	{
		return;
	}

	if (s_nativeVision.driverHidden && (gGT->drivers[0] != NULL) && (gGT->drivers[0]->instSelf != NULL))
	{
		gGT->drivers[0]->instSelf->flags = s_nativeVision.savedDriverFlags;
		s_nativeVision.driverHidden = 0;
	}

	gGT->pushBuffer[0] = s_nativeVision.savedPushBuffer;
	gGT->pushBuffer[0].ptrOT = s_nativeVision.layerOT[NATIVE_VISION_LAYER_HUD];
	gGT->pushBuffer_UI.ptrOT = s_nativeVision.layerOT[NATIVE_VISION_LAYER_HUD];
	gGT->backBuffer->otMem.uiOT = s_nativeVision.layerOT[NATIVE_VISION_LAYER_HUD];
	s_nativeVision.currentEye = -1;
	s_nativeVision.currentLayer = NATIVE_VISION_LAYER_HUD;
	PushBuffer_UpdateFrustum(&gGT->pushBuffer[0]);
}

void NativeVision_EndStereoFrame(struct GameTracker *gGT)
{
	if ((gGT == NULL) || !s_nativeVision.frameActive)
	{
		return;
	}

	if (s_nativeVision.driverHidden && (gGT->drivers[0] != NULL) && (gGT->drivers[0]->instSelf != NULL))
	{
		gGT->drivers[0]->instSelf->flags = s_nativeVision.savedDriverFlags;
	}

	u32 *rootOT = s_nativeVision.layerOT[NATIVE_VISION_LAYER_LEFT];
	/* HUD fades are stateful; keep the once-per-frame result when restoring camera state. */
	s_nativeVision.savedPushBuffer.fadeFromBlack_currentValue = gGT->pushBuffer[0].fadeFromBlack_currentValue;
	s_nativeVision.savedPushBuffer.fadeFromBlack_desiredResult = gGT->pushBuffer[0].fadeFromBlack_desiredResult;
	s_nativeVision.savedPushBuffer.fade_step = gGT->pushBuffer[0].fade_step;
	gGT->pushBuffer[0] = s_nativeVision.savedPushBuffer;
	gGT->pushBuffer[0].ptrOT = rootOT;
	gGT->pushBuffer_UI.ptrOT = s_nativeVision.savedUiOT;
	gGT->backBuffer->otMem.uiOT = s_nativeVision.savedOtMemUiOT;
	s_nativeVision.currentEye = -1;
	s_nativeVision.currentLayer = NATIVE_VISION_LAYER_HUD;
	s_nativeVision.frameMode = NATIVE_VISION_MODE_WINDOW;
	s_nativeVision.driverHidden = 0;
	s_nativeVision.frameActive = 0;
}

void NativeVision_AddLayerMarker(struct GameTracker *gGT, int layer)
{
	if ((gGT == NULL) || !s_nativeVision.frameActive || (layer < 0) || (layer >= NATIVE_VISION_LAYER_COUNT))
	{
		return;
	}

	struct NativeVisionLayerPacket *packet = (struct NativeVisionLayerPacket *)gGT->backBuffer->primMem.cursor;
	if ((u8 *)(packet + 1) > (u8 *)gGT->backBuffer->primMem.guardEnd)
	{
		return;
	}
	memset(packet, 0, sizeof(*packet));
	setlen(packet, 2);
	packet->code = 0xb3;
	packet->layer = (u32)layer;
	AddPrim(&s_nativeVision.layerOT[layer][0x3ff], packet);
	gGT->backBuffer->primMem.cursor = packet + 1;
}

void NativeVision_AdjustGeomOffset(const struct PushBuffer *pb, int *x, int *y)
{
	if ((pb == NULL) || (x == NULL) || (y == NULL) || !s_nativeVision.frameActive || (s_nativeVision.currentEye < 0))
	{
		return;
	}

	float convergence = s_nativeVision.frameTracking.convergenceMetres;
	float eyeX = s_nativeVision.frameTracking.eyePositionMetres[s_nativeVision.currentEye][0];
	if (isfinite(convergence) && (convergence >= 0.05f) && isfinite(eyeX))
	{
		*x += (int)lrintf(((float)pb->distanceToScreen_PREV * eyeX) / convergence);
		float eyeY = s_nativeVision.frameTracking.eyePositionMetres[s_nativeVision.currentEye][1];
		if (isfinite(eyeY))
		{
			*y += (int)lrintf(((float)pb->distanceToScreen_PREV * eyeY) / convergence);
		}
	}
}

int NativeVision_IsCockpitPass(void)
{
	return s_nativeVision.frameActive && (s_nativeVision.currentEye >= 0) &&
	       (s_nativeVision.frameMode == NATIVE_VISION_MODE_COCKPIT);
}

int NativeVision_RunStereoMathSelfTest(void)
{
	struct NativeVisionTrackingFrame frame = NativeVision_DefaultTracking();
	if ((frame.eyePositionMetres[0][0] >= 0.0f) || (frame.eyePositionMetres[1][0] <= 0.0f))
	{
		return 1;
	}
	if (fabsf((frame.eyePositionMetres[1][0] - frame.eyePositionMetres[0][0]) - NATIVE_VISION_DEFAULT_IPD_METRES) > 0.00001f)
	{
		return 1;
	}

	struct PushBuffer pb;
	memset(&pb, 0, sizeof(pb));
	pb.distanceToScreen_PREV = 256;
	s_nativeVision.frameTracking = frame;
	s_nativeVision.frameActive = 1;
	s_nativeVision.currentEye = 0;
	int left = 256;
	int y = 108;
	NativeVision_AdjustGeomOffset(&pb, &left, &y);
	s_nativeVision.currentEye = 1;
	int right = 256;
	NativeVision_AdjustGeomOffset(&pb, &right, &y);
	s_nativeVision.frameActive = 0;
	s_nativeVision.currentEye = -1;

	if (!((left < 256) && (right > 256) && ((256 - left) == (right - 256))))
	{
		return 1;
	}
	printf("[CTR Vision] stereo math self-test passed: ipd=0.064 convergence=4.000 shifts=symmetric\n");
	return 0;
}

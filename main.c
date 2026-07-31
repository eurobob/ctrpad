#define _CRT_SECURE_NO_WARNINGS

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <io.h>
#include "platform/native_win32.h"
#else
#include <unistd.h>
#endif

#include <SDL3/SDL.h>
#if !defined(SDL_PLATFORM_IOS)
#define SDL_MAIN_HANDLED
#endif
#include <SDL3/SDL_main.h>
#define _EnterCriticalSection(x)
#define EnterCriticalSection(x)
#define ExitCriticalSection()

#include "platform/native_assets.h"
#include "platform/native_asset_relocation.h"
#include "platform/native_guest_ref.h"
#include "platform/native_log.h"
#include "platform/native_memory.h"
#include "platform/native_perf.h"
#include "platform/native_replay_scheduler.h"
#include "platform/native_savestate.h"
#include "platform/native_state_digest.h"

#include <platform.h>

#include "game/game_unity.h"

#include "game/zGlobal_RDATA.c"
#include "game/zGlobal_DATA.c"
#include "game/zGlobal_SDATA.c"

#undef RECT

#include "platform/native_disc_image.c"
#include "platform/native_assets.c"
#include "platform/native_asset_ref.c"
#include "platform/native_asset_relocation.c"
#include "platform/native_audio.c"
#include "platform/native_memory.c"
#include "platform/native_checkpoint.c"
#include "platform/native_checkpoint_file.c"
#include "platform/native_cd.c"
#include "platform/native_guest_ref.c"
#include "platform/native_gpu_links.c"
#include "platform/native_gpu.c"
#include "platform/native_gte_core.c"
#include "platform/native_glad.c"
#include "platform/native_input.c"
#include "platform/native_inline_c.c"
#include "platform/native_libapi.c"
#include "platform/native_libetc.c"
#include "platform/native_libgte.c"
#include "platform/native_libgpu.c"
#include "platform/native_libpad.c"
#include "platform/native_libspu.c"
#include "platform/native_log.c"
#include "platform/native_memcard.c"
#include "platform/native_memcard_adapter.c"
#include "platform/native_perf.c"
#include "platform/native_platform.c"
#include "platform/native_replay_scheduler.c"
#include "platform/native_renderer.c"
#include "platform/native_savestate.c"
#include "platform/native_state.c"
#include "platform/native_state_digest.c"
#include "platform/native_str.c"

#ifndef CC
#if defined(__GNUC__)
#if _WIN32
#ifndef __clang__
#define CC "MINGW-GCC"
#else
#define CC "MINGW-CLANG"
#endif
#else
#ifndef __clang__
#define CC "GCC"
#else
#define CC "CLANG"
#endif
#endif
#elif defined(_MSC_VER)
#define CC "MSVC"
#else
#define CC "Unknown"
#endif
#endif

#ifndef CTR_NATIVE_VERSION
#define CTR_NATIVE_VERSION "0.0.0-dev"
#endif

#ifndef CTR_NATIVE_BUILD_ID
#define CTR_NATIVE_BUILD_ID "unknown"
#endif

static int NativeConsole_ShouldPauseOnError(void)
{
#if defined(_WIN32)
	DWORD consoleProcesses[2];
	DWORD consoleProcessCount;

	if (GetConsoleWindow() == NULL)
		return 0;

	consoleProcessCount = GetConsoleProcessList(consoleProcesses, (DWORD)(sizeof(consoleProcesses) / sizeof(consoleProcesses[0])));
	return (consoleProcessCount == 1) && (consoleProcesses[0] == GetCurrentProcessId());
#else
	return 0;
#endif
}

static s32 NativeConsole_Return(const u32 result)
{
	if ((result != 0) && NativeConsole_ShouldPauseOnError())
	{
		fflush(stdout);
		fflush(stderr);
		fprintf(stderr, "\n[CTR Native] Press Enter to close this window...");
		fflush(stderr);

		while (getchar() != '\n' && !feof(stdin))
		{
		}
	}

	return (s32)result;
}

// TODO(aalhendi): just make an argparser?
static int NativeArg_IsVersion(const char *arg)
{
	return (arg != NULL) && ((strcmp(arg, "--version") == 0) || (strcmp(arg, "-v") == 0));
}

static int NativeArg_IsStateDigestSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-state-digest") == 0);
}

static int NativeArg_IsReplayGateSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-replay-gate") == 0);
}

static int NativeArg_IsGuestRefSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-guest-ref") == 0);
}

static int NativeArg_IsAssetRelocationSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-asset-relocation") == 0);
}

static int NativeArg_IsInputSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-input") == 0);
}

static int NativeArg_IsCollisionScratchSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-collision-scratch") == 0);
}

static int NativeArg_IsVehicleConstantsSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-vehicle-constants") == 0);
}

static int NativeArg_IsVsQuipOffsetsSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-vs-quip-offsets") == 0);
}

static int NativeArg_IsRenderListsSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-render-lists") == 0);
}

static int NativeArg_IsRedBeakerLayoutSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-red-beaker-layout") == 0);
}

static int NativeArg_IsAudioStateAlignmentSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-audio-state-alignment") == 0);
}

static int NativeArg_IsAudioMixerSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-audio-mixer") == 0);
}

static int NativeArg_IsRendererDialectSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-renderer-dialect") == 0);
}

static int NativeArg_IsCheckpointPointerValidationSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-checkpoint-pointer-validation") == 0);
}

static int NativeArg_IsVehicleLapCheckpointBoundsSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-vehicle-lap-checkpoint-bounds") == 0);
}

static int NativeArg_IsPotionEmitterLayoutSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-potion-emitter-layout") == 0);
}

static int NativeArg_IsCutsceneParticleEmitterLayoutSelfTest(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--self-test-cutscene-particle-emitter-layout") == 0);
}

static int NativeArg_IsScrapbookSTRProbe(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--probe-str-scrapbook") == 0);
}

static int NativeArg_IsScrapbookSTRPresentProbe(const char *arg)
{
	return (arg != NULL) && (strcmp(arg, "--probe-str-scrapbook-present") == 0);
}

static int NativeArg_ParseScrapbookSTRProbeFrames(const char *text, s32 *frameCount)
{
	char *end = NULL;
	long value;

	if ((text == NULL) || (text[0] == '\0') || (frameCount == NULL))
	{
		return 0;
	}

	errno = 0;
	value = strtol(text, &end, 10);
	if ((errno != 0) || (end == text) || (*end != '\0') || (value <= 0) || (value > INT_MAX))
	{
		return 0;
	}

	*frameCount = (s32)value;
	return 1;
}

int main(int argc, char *argv[])
{
	s32 scrapbookSTRProbeFrames = 0;
	s32 scrapbookSTRPresentProbeFrames = 0;
	const char *scrapbookSTRPresentProbePath = NULL;

	for (int argIndex = 1; argIndex < argc; argIndex++)
	{
		if (NativeArg_IsVersion(argv[argIndex]))
		{
			printf("CTR Native %s (%s)\n", CTR_NATIVE_VERSION, CTR_NATIVE_BUILD_ID);
			return 0;
		}
		if (NativeArg_IsStateDigestSelfTest(argv[argIndex]))
		{
			return NativeStateDigest_RunSelfTest();
		}
		if (NativeArg_IsReplayGateSelfTest(argv[argIndex]))
		{
			return NativeReplayScheduler_RunSelfTest();
		}
		if (NativeArg_IsGuestRefSelfTest(argv[argIndex]))
		{
			return NativeGuestRef_RunSelfTest();
		}
		if (NativeArg_IsAssetRelocationSelfTest(argv[argIndex]))
		{
			return NativeAssetRelocation_RunSelfTest();
		}
		if (NativeArg_IsInputSelfTest(argv[argIndex]))
		{
			return Platform_InputRunSelfTest();
		}
		if (NativeArg_IsCollisionScratchSelfTest(argv[argIndex]))
		{
			return COLL_Scratch_RunSelfTest();
		}
		if (NativeArg_IsVehicleConstantsSelfTest(argv[argIndex]))
		{
			return VehBirth_RunConstOffsetSelfTest();
		}
		if (NativeArg_IsVsQuipOffsetsSelfTest(argv[argIndex]))
		{
			return UI_VsQuipRunOffsetSelfTest();
		}
		if (NativeArg_IsRenderListsSelfTest(argv[argIndex]))
		{
			return RenderLists_RunHostLayoutSelfTest();
		}
		if (NativeArg_IsRedBeakerLayoutSelfTest(argv[argIndex]))
		{
			return RedBeaker_RunHostLayoutSelfTest();
		}
		if (NativeArg_IsAudioStateAlignmentSelfTest(argv[argIndex]))
		{
			return NativeAudio_RunStateAlignmentSelfTest();
		}
		if (NativeArg_IsAudioMixerSelfTest(argv[argIndex]))
		{
			return NativeAudio_RunMixerSelfTest();
		}
		if (NativeArg_IsRendererDialectSelfTest(argv[argIndex]))
		{
			return NativeRenderer_RunDialectSelfTest();
		}
		if (NativeArg_IsCheckpointPointerValidationSelfTest(argv[argIndex]))
		{
			return NativeCheckpoint_RunPointerValidationSelfTest();
		}
		if (NativeArg_IsVehicleLapCheckpointBoundsSelfTest(argv[argIndex]))
		{
			return VehLap_RunCheckpointBoundsSelfTest();
		}
		if (NativeArg_IsPotionEmitterLayoutSelfTest(argv[argIndex]))
		{
			return RB_Explosion_RunPotionEmitterSelfTest();
		}
		if (NativeArg_IsCutsceneParticleEmitterLayoutSelfTest(argv[argIndex]))
		{
			return OVR233_RunParticleEmitterLayoutSelfTest();
		}
		if (NativeArg_IsScrapbookSTRProbe(argv[argIndex]))
		{
			if ((scrapbookSTRProbeFrames != 0) || (argIndex + 1 >= argc) ||
			    !NativeArg_ParseScrapbookSTRProbeFrames(argv[argIndex + 1], &scrapbookSTRProbeFrames))
			{
				fprintf(stderr, "[CTR STR] --probe-str-scrapbook requires one positive frame count\n");
				return 1;
			}
			argIndex++;
		}
		if (NativeArg_IsScrapbookSTRPresentProbe(argv[argIndex]))
		{
			if ((scrapbookSTRPresentProbeFrames != 0) || (argIndex + 2 >= argc) ||
			    !NativeArg_ParseScrapbookSTRProbeFrames(argv[argIndex + 1], &scrapbookSTRPresentProbeFrames) ||
			    (argv[argIndex + 2][0] == '\0'))
			{
				fprintf(stderr, "[CTR STR] --probe-str-scrapbook-present requires one positive frame count and screenshot path\n");
				return 1;
			}
			scrapbookSTRPresentProbePath = argv[argIndex + 2];
			argIndex += 2;
		}
	}

	if ((scrapbookSTRProbeFrames != 0) && (scrapbookSTRPresentProbeFrames != 0))
	{
		fprintf(stderr, "[CTR STR] select only one scrapbook probe mode\n");
		return 1;
	}

	printf("[CTR Native] Starting...\n");
	fflush(stdout);

	const char *sdlBasePath = SDL_GetBasePath();
	printf("[CTR Native] SDL base path: %s\n", sdlBasePath ? sdlBasePath : "(null)");
	fflush(stdout);

#if defined(CTR_INTERNAL)
	NativeReplayScheduler_SetExecutableIdentity(argv[0], sdlBasePath);
#endif

	if (!NativeAssets_Init(sdlBasePath))
	{
		fprintf(stderr, "[CTR Native] Failed to initialize asset paths.\n");
		return NativeConsole_Return(1);
	}

	printf("[CTR Native] Version: %s (%s)\n", CTR_NATIVE_VERSION, CTR_NATIVE_BUILD_ID);
	printf("[CTR Native] Built with: " CC "\n");
	printf("[CTR Native] Base: %s\n", NativeAssets_GetBaseDir());
	printf("[CTR Native] Assets: %s\n", NativeAssets_GetAssetDir());
	fflush(stdout);

	if (chdir(NativeAssets_GetBaseDir()) != 0)
	{
		fprintf(stderr, "[CTR Native] Failed to enter base directory: %s\n", NativeAssets_GetBaseDir());
		return NativeConsole_Return(1);
	}

	if (!NativeAssets_Validate())
	{
		return NativeConsole_Return(1);
	}

	if (scrapbookSTRProbeFrames != 0)
	{
		return NativeConsole_Return((u32)NativeSTR_RunScrapbookProbe(scrapbookSTRProbeFrames));
	}
	if (scrapbookSTRPresentProbeFrames != 0)
	{
		return NativeConsole_Return(
		    (u32)NativeSTR_RunScrapbookPresentProbe(scrapbookSTRPresentProbeFrames, scrapbookSTRPresentProbePath));
	}

#if defined(CTR_INTERNAL)
	if (NativeReplayScheduler_PrepareReportFromArgs(argc, argv) != 0)
	{
		return NativeConsole_Return(1);
	}
#endif

#ifdef USE_16BY9
	printf("[CTR Native] Widescreen\n");
	Platform_Init("Crash Team Racing", 1280, 720);
#else
	printf("[CTR Native] 4:3\n");
	Platform_Init("Crash Team Racing", 800, 600);
#endif
	if (!Platform_IsInitialized())
	{
		return NativeConsole_Return(1);
	}

#if defined(CTR_INTERNAL)
	if (NativePerf_ConfigureFromArgs(argc, argv) != 0)
	{
		Platform_LogFlush();
		Platform_Shutdown();
		return NativeConsole_Return(1);
	}
#endif

	Platform_InitScratchpad();
	Platform_RepairResidentPointers(0);

#if defined(CTR_INTERNAL)
	if (NativeReplayScheduler_ConfigureFromArgs(argc, argv) != 0)
	{
		Platform_LogFlush();
		Platform_Shutdown();
		return NativeConsole_Return(1);
	}
#else
	(void)argc;
	(void)argv;
#endif

	const int result = CTR_Main();

	Platform_Shutdown();
#if defined(CTR_INTERNAL)
	const int replayExitStatus = NativeReplayScheduler_GetExitStatus();
	if (replayExitStatus != 0)
	{
		return NativeConsole_Return((u32)replayExitStatus);
	}
#endif
	return NativeConsole_Return(result);
}

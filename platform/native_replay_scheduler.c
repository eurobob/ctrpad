#include "platform/native_replay_scheduler.h"

#include <macros.h>

#if defined(CTR_INTERNAL)
#include "platform/native_audio.h"
#include "platform/native_checkpoint.h"
#include "platform/native_checkpoint_file.h"
#include "platform/native_gpu.h"
#include "platform/native_input.h"
#include "platform/native_log.h"
#include "platform/native_memcard.h"
#include "platform/native_path.h"
#include "platform/native_state.h"

#include <platform.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <direct.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

// NOTE(aalhendi): Little-endian tags `CTRR`/`RFRM` = CTR native Replay.
#define NATIVE_REPLAY_FILE_MAGIC                 0x52525443u
#define NATIVE_REPLAY_FRAME_MAGIC                0x4d524652u
#define NATIVE_REPLAY_FILE_VERSION               4u
#define NATIVE_REPLAY_INPUT_MIN_VERSION          2u
#define NATIVE_REPLAY_FNV_OFFSET                 2166136261u
#define NATIVE_REPLAY_FNV_PRIME                  16777619u
#define NATIVE_REPLAY_FNV64_OFFSET               UINT64_C(14695981039346656037)
#define NATIVE_REPLAY_FNV64_PRIME                UINT64_C(1099511628211)
#define NATIVE_REPLAY_CHECKPOINT_INTERVAL_FRAMES 300u
#define NATIVE_REPLAY_MAX_VSYNC_PACKETS          64u
#define NATIVE_REPLAY_VSYNC_COUNT_MASK           0xffffu
#define NATIVE_REPLAY_VSYNC_PREFRAME_SHIFT       16u
#define NATIVE_REPLAY_VSYNC_PACKET_VALUE_MASK    0x00ffu
#define NATIVE_REPLAY_VSYNC_PACKET_REPEAT_SHIFT  8u
#define NATIVE_REPLAY_VSYNC_PACKET_REPEAT_MAX    256u
#define NATIVE_REPLAY_BUILD_ID_BYTES             64u
#define NATIVE_REPLAY_PLATFORM_ID_BYTES          32u
#define NATIVE_REPLAY_DEFAULT_REPORT_ROOT        "debug/reports"
#define NATIVE_REPLAY_REPORT_REPLAY_NAME         "input.ctrreplay"
#define NATIVE_REPLAY_REPORT_CHECKPOINT_NAME     "state.ctrstates"
#define NATIVE_REPLAY_REPORT_MEMCARD_SEED_NAME   "memcard.seed"
#define NATIVE_REPLAY_RECORDING_MEMCARD_NAME     "memcard.recording"
#define NATIVE_REPLAY_PLAYBACK_MEMCARD_NAME      "memcard.playback"
#define NATIVE_REPLAY_REPORT_METADATA_NAME       "metadata.txt"
#define NATIVE_REPLAY_REPORT_LOG_NAME            "ctr-native.log"
#define NATIVE_REPLAY_EXIT_FAILURE                1
#define NATIVE_REPLAY_EXIT_DIVERGENCE             2

enum NativeReplaySchedulerMode
{
	NATIVE_REPLAY_MODE_NONE = 0,
	NATIVE_REPLAY_MODE_ARMED,
	NATIVE_REPLAY_MODE_RECORD,
	NATIVE_REPLAY_MODE_PLAYBACK
};

enum NativeReplayCheckpointPolicy
{
	NATIVE_REPLAY_CHECKPOINT_POLICY_ROLLING = 0,
	NATIVE_REPLAY_CHECKPOINT_POLICY_BOOTSTRAP_ONLY
};

struct NativeReplayFileHeader
{
	u32 magic;
	u32 version;
	u32 headerSize;
	u32 frameRecordSize;
	u32 frameCount;
	u32 checkpointCount;
	u32 checkpointSize;
	u32 nativeStateSize;
	u32 identityChecksum;
	u32 checkpointPolicy;
	char buildId[NATIVE_REPLAY_BUILD_ID_BYTES];
	char platformId[NATIVE_REPLAY_PLATFORM_ID_BYTES];
	u32 executableFingerprintLow;
	u32 executableFingerprintHigh;
	u32 reserved;
};

struct NativeReplayFrameRecord
{
	u32 magic;
	u32 replayFrame;
	struct NativeReplaySchedulerFrameInfo beginInfo;
	struct NativeReplaySchedulerFrameInfo endInfo;
	struct PlatformInputPadSnapshot pads[PLATFORM_INPUT_PAD_COUNT];
	u32 vblankTotal;
	u32 vblankPacketCount;
	u16 vblankPackets[NATIVE_REPLAY_MAX_VSYNC_PACKETS];
	u32 padChecksum;
	u32 recordChecksum;
};

CTR_STATIC_ASSERT(sizeof(struct NativeReplaySchedulerFrameInfo) == 120);
CTR_STATIC_ASSERT(sizeof(struct NativeReplayFileHeader) == 148);
CTR_STATIC_ASSERT(sizeof(struct NativeReplayFrameRecord) == 440);

global_variable enum NativeReplaySchedulerMode s_mode;
global_variable u32 s_replayFrame;
global_variable s32 s_beginOpen;
global_variable s32 s_divergenceLogged;
global_variable FILE *s_file;
global_variable struct NativeReplayFileHeader s_header;
global_variable struct NativeReplayFrameRecord s_pendingRecord;
global_variable FILE *s_inputFile;
global_variable struct NativeReplayFileHeader s_inputHeader;
global_variable struct NativeReplayFrameRecord s_inputRecord;
global_variable char *s_inputReplayPath;
global_variable struct NativeCheckpointFileWriter s_checkpointWriter;
global_variable char *s_checkpointPath;
global_variable u8 *s_checkpointPayload;
global_variable int s_checkpointPayloadSize;
global_variable u32 s_nextCheckpointFrame;
global_variable u32 s_checkpointIndex;
global_variable s32 s_checkpointWriterOpen;
global_variable s32 s_restoreBootstrapCheckpoint;
global_variable u32 s_restoreCheckpointIndex;
global_variable s32 s_renderTraceEnabled;
global_variable u32 s_renderTraceFrame;
global_variable s32 s_frameTimingConsumed;
global_variable u32 s_frameVBlankTotal;
global_variable u32 s_frameVBlankPacketCount;
global_variable u32 s_frameVBlankPacketRepeatCursor;
global_variable u32 s_framePreVBlankPacketCount;
global_variable s32 s_vblankPacketOverflow;
global_variable s32 s_vblankPlaybackMismatch;
global_variable u32 s_preFrameVBlankTotal;
global_variable u32 s_preFrameVBlankPacketCount;
global_variable u16 s_preFrameVBlankPackets[NATIVE_REPLAY_MAX_VSYNC_PACKETS];
global_variable s32 s_preFrameVBlankPacketOverflow;
global_variable u32 s_inputVBlankPacketCursor;
global_variable u32 s_inputVBlankPacketRepeatCursor;
global_variable s32 s_inputRecordReady;
global_variable s32 s_inputCompleteTiming;
global_variable s32 s_inputVBlankPlaybackMismatch;
global_variable enum NativeReplayCheckpointPolicy s_checkpointPolicy = NATIVE_REPLAY_CHECKPOINT_POLICY_ROLLING;
global_variable s32 s_startRequested;
global_variable s32 s_stopRequested;
global_variable s32 s_reportCompleted;
global_variable s32 s_reportManualStart;
global_variable s32 s_reportEnabled;
global_variable char *s_reportDir;
global_variable char *s_reportReplayPath;
global_variable char *s_reportCheckpointPath;
global_variable char *s_reportMemcardSeedPath;
global_variable char *s_reportMemcardRecordingPath;
global_variable char *s_reportMetadataPath;
global_variable char *s_reportLogPath;
global_variable char *s_playbackMemcardPath;
global_variable s32 s_memcardSandboxActive;
global_variable s32 s_recordStartDeferredLogged;
global_variable s32 s_exitStatus;
global_variable s32 s_testPerturbEnabled;
global_variable s32 s_testPerturbApplied;
global_variable u32 s_testPerturbFrame;
global_variable s32 s_driver0ActiveState;
global_variable s32 s_raceDriver0ActiveState;
global_variable u64 s_executableFingerprint;
global_variable s32 s_executableFingerprintReady;

internal u32 NativeReplayScheduler_RecordPreFrameVSyncPacketCount(u32 version, const struct NativeReplayFrameRecord *record);
internal u32 NativeReplayScheduler_VSyncPacketValue(u32 version, u16 packet);
internal u32 NativeReplayScheduler_VSyncPacketRepeatCount(u32 version, u16 packet);

internal void NativeReplayScheduler_SetFailure(void)
{
	if (s_exitStatus == 0)
	{
		s_exitStatus = NATIVE_REPLAY_EXIT_FAILURE;
	}
}

internal int NativeReplayScheduler_RuntimeFailure(void)
{
	NativeReplayScheduler_SetFailure();
	return 1;
}

internal void NativeReplayScheduler_CheckPlaybackComplete(void)
{
	if ((s_mode == NATIVE_REPLAY_MODE_PLAYBACK) && (s_exitStatus == 0) && (s_replayFrame < s_header.frameCount))
	{
		Platform_Log("[CTR Replay] playback stopped early at frame %u of %u\n", s_replayFrame, s_header.frameCount);
		NativeReplayScheduler_SetFailure();
	}
}

internal void NativeReplayScheduler_ResetVSyncPackets(void)
{
	s_frameVBlankTotal = 0;
	s_frameVBlankPacketCount = 0;
	s_frameVBlankPacketRepeatCursor = 0;
	s_framePreVBlankPacketCount = 0;
	s_vblankPacketOverflow = 0;
	s_vblankPlaybackMismatch = 0;
}

internal void NativeReplayScheduler_ResetPreFrameVSyncPackets(void)
{
	s_preFrameVBlankTotal = 0;
	s_preFrameVBlankPacketCount = 0;
	s_preFrameVBlankPacketOverflow = 0;
}

internal void NativeReplayScheduler_RecordEmittedVSyncPacket(int emittedVBlanks)
{
	u32 *total;
	u32 *count;
	u32 firstMergeableEntry;
	s32 *overflow;
	u16 *packets;

	if (emittedVBlanks <= 0)
	{
		return;
	}

	if (s_beginOpen != 0)
	{
		total = &s_frameVBlankTotal;
		count = &s_frameVBlankPacketCount;
		firstMergeableEntry = s_framePreVBlankPacketCount;
		overflow = &s_vblankPacketOverflow;
		packets = s_pendingRecord.vblankPackets;
	}
	else
	{
		total = &s_preFrameVBlankTotal;
		count = &s_preFrameVBlankPacketCount;
		firstMergeableEntry = 0;
		overflow = &s_preFrameVBlankPacketOverflow;
		packets = s_preFrameVBlankPackets;
	}

	if ((emittedVBlanks > (int)NATIVE_REPLAY_VSYNC_PACKET_VALUE_MASK) ||
	    (*total > UINT32_MAX - (u32)emittedVBlanks))
	{
		*overflow = 1;
		return;
	}

	if (*count > firstMergeableEntry)
	{
		u16 *lastPacket = &packets[*count - 1u];
		u32 lastValue = *lastPacket & NATIVE_REPLAY_VSYNC_PACKET_VALUE_MASK;
		u32 lastRepeat = (*lastPacket >> NATIVE_REPLAY_VSYNC_PACKET_REPEAT_SHIFT) + 1u;

		if ((lastValue == (u32)emittedVBlanks) && (lastRepeat < NATIVE_REPLAY_VSYNC_PACKET_REPEAT_MAX))
		{
			*lastPacket += (u16)(1u << NATIVE_REPLAY_VSYNC_PACKET_REPEAT_SHIFT);
			*total += (u32)emittedVBlanks;
			return;
		}
	}

	if (*count >= NATIVE_REPLAY_MAX_VSYNC_PACKETS)
	{
		*overflow = 1;
		return;
	}

	packets[*count] = (u16)(u32)emittedVBlanks;
	(*count)++;
	*total += (u32)emittedVBlanks;
}

internal void NativeReplayScheduler_BeginRecordedFrameVSync(void)
{
	NativeReplayScheduler_ResetVSyncPackets();

	s_frameVBlankTotal = s_preFrameVBlankTotal;
	s_frameVBlankPacketCount = s_preFrameVBlankPacketCount;
	s_framePreVBlankPacketCount = s_preFrameVBlankPacketCount;
	s_vblankPacketOverflow = s_preFrameVBlankPacketOverflow;
	if (s_preFrameVBlankPacketCount != 0)
	{
		memcpy(s_pendingRecord.vblankPackets, s_preFrameVBlankPackets,
		       s_preFrameVBlankPacketCount * sizeof(s_preFrameVBlankPackets[0]));
	}

	NativeReplayScheduler_ResetPreFrameVSyncPackets();
}

internal void NativeReplayScheduler_BeginPlaybackFrameVSync(u32 version, const struct NativeReplayFrameRecord *record)
{
	u32 preFrameCount;

	NativeReplayScheduler_ResetVSyncPackets();
	preFrameCount = NativeReplayScheduler_RecordPreFrameVSyncPacketCount(version, record);
	s_framePreVBlankPacketCount = preFrameCount;
	s_frameVBlankPacketCount = preFrameCount;
	for (u32 i = 0; i < preFrameCount; i++)
	{
		s_frameVBlankTotal += NativeReplayScheduler_VSyncPacketValue(version, record->vblankPackets[i]) *
		                     NativeReplayScheduler_VSyncPacketRepeatCount(version, record->vblankPackets[i]);
	}
}

internal void NativeReplayScheduler_ResetSessionState(void)
{
	s_replayFrame = 0;
	s_beginOpen = 0;
	s_divergenceLogged = 0;
	s_frameTimingConsumed = 0;
	s_stopRequested = 0;
	s_recordStartDeferredLogged = 0;
	s_exitStatus = 0;
	s_testPerturbApplied = 0;
	s_driver0ActiveState = -1;
	s_raceDriver0ActiveState = -1;
	s_inputVBlankPacketCursor = 0;
	s_inputVBlankPacketRepeatCursor = 0;
	s_inputRecordReady = 0;
	s_inputCompleteTiming = 0;
	s_inputVBlankPlaybackMismatch = 0;
	NativeReplayScheduler_ResetVSyncPackets();
	NativeReplayScheduler_ResetPreFrameVSyncPackets();
}

internal u32 NativeReplayScheduler_Fnv1a(const void *data, u32 size)
{
	const u8 *bytes = (const u8 *)data;
	u32 hash = NATIVE_REPLAY_FNV_OFFSET;
	u32 i;

	for (i = 0; i < size; i++)
	{
		hash ^= bytes[i];
		hash *= NATIVE_REPLAY_FNV_PRIME;
	}

	return hash;
}

internal u32 NativeReplayScheduler_Fnv1aStep(u32 hash, const void *data, u32 size)
{
	const u8 *bytes = (const u8 *)data;
	u32 i;

	for (i = 0; i < size; i++)
	{
		hash ^= bytes[i];
		hash *= NATIVE_REPLAY_FNV_PRIME;
	}

	return hash;
}

internal s32 NativeReplayScheduler_HashExecutableFile(const char *path, u64 *hashOut)
{
	u8 buffer[64 * 1024];
	u64 hash = NATIVE_REPLAY_FNV64_OFFSET;
	FILE *file;
	size_t bytesRead;

	if ((path == NULL) || (path[0] == '\0') || (hashOut == NULL))
	{
		return 0;
	}

	file = fopen(path, "rb");
	if (file == NULL)
	{
		return 0;
	}

	while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) != 0)
	{
		for (size_t i = 0; i < bytesRead; i++)
		{
			hash ^= buffer[i];
			hash *= NATIVE_REPLAY_FNV64_PRIME;
		}
	}

	if (ferror(file) != 0)
	{
		fclose(file);
		return 0;
	}
	if (fclose(file) != 0)
	{
		return 0;
	}

	*hashOut = hash;
	return 1;
}

internal const char *NativeReplayScheduler_PathBasename(const char *path)
{
	const char *basename = path;

	if (path == NULL)
	{
		return NULL;
	}

	for (const char *cursor = path; *cursor != '\0'; cursor++)
	{
		if ((*cursor == '/') || (*cursor == '\\'))
		{
			basename = cursor + 1;
		}
	}

	return basename;
}

int NativeReplayScheduler_SetExecutableIdentity(const char *argv0, const char *executableBasePath)
{
	const char *basename;
	char *candidate = NULL;
	size_t baseLength;
	size_t basenameLength;
	size_t separatorLength;
	u64 hash;

	s_executableFingerprint = 0;
	s_executableFingerprintReady = 0;

	if (NativeReplayScheduler_HashExecutableFile(argv0, &hash))
	{
		s_executableFingerprint = hash;
		s_executableFingerprintReady = 1;
		return 1;
	}

	basename = NativeReplayScheduler_PathBasename(argv0);
	if ((basename == NULL) || (basename[0] == '\0') || (executableBasePath == NULL) || (executableBasePath[0] == '\0'))
	{
		return 0;
	}

	baseLength = strlen(executableBasePath);
	basenameLength = strlen(basename);
	separatorLength = ((executableBasePath[baseLength - 1u] == '/') || (executableBasePath[baseLength - 1u] == '\\')) ? 0u : 1u;
	if (baseLength > SIZE_MAX - separatorLength - basenameLength - 1u)
	{
		return 0;
	}

	candidate = (char *)malloc(baseLength + separatorLength + basenameLength + 1u);
	if (candidate == NULL)
	{
		return 0;
	}
	memcpy(candidate, executableBasePath, baseLength);
	if (separatorLength != 0)
	{
		candidate[baseLength] = '/';
	}
	memcpy(&candidate[baseLength + separatorLength], basename, basenameLength + 1u);

	if (NativeReplayScheduler_HashExecutableFile(candidate, &hash))
	{
		s_executableFingerprint = hash;
		s_executableFingerprintReady = 1;
	}
	free(candidate);
	return s_executableFingerprintReady;
}

internal u32 NativeReplayScheduler_PadChecksum(const struct PlatformInputPadSnapshot *pads)
{
	return NativeReplayScheduler_Fnv1a(pads, sizeof(struct PlatformInputPadSnapshot) * PLATFORM_INPUT_PAD_COUNT);
}

internal u32 NativeReplayScheduler_RecordChecksum(const struct NativeReplayFrameRecord *record)
{
	struct NativeReplayFrameRecord checksumRecord = *record;

	checksumRecord.recordChecksum = 0;
	return NativeReplayScheduler_Fnv1a(&checksumRecord, sizeof(checksumRecord));
}

internal u32 NativeReplayScheduler_RecordVSyncPacketCount(u32 version, const struct NativeReplayFrameRecord *record)
{
	if (record == NULL)
	{
		return 0;
	}
	if (version >= 4u)
	{
		return record->vblankPacketCount & NATIVE_REPLAY_VSYNC_COUNT_MASK;
	}
	return record->vblankPacketCount;
}

internal u32 NativeReplayScheduler_RecordPreFrameVSyncPacketCount(u32 version, const struct NativeReplayFrameRecord *record)
{
	if ((record == NULL) || (version < 4u))
	{
		return 0;
	}
	return record->vblankPacketCount >> NATIVE_REPLAY_VSYNC_PREFRAME_SHIFT;
}

internal u32 NativeReplayScheduler_EncodeVSyncPacketCounts(u32 preFrameCount, u32 packetCount)
{
	return (preFrameCount << NATIVE_REPLAY_VSYNC_PREFRAME_SHIFT) | packetCount;
}

internal u32 NativeReplayScheduler_VSyncPacketValue(u32 version, u16 packet)
{
	if (version >= 4u)
	{
		return packet & NATIVE_REPLAY_VSYNC_PACKET_VALUE_MASK;
	}
	return packet;
}

internal u32 NativeReplayScheduler_VSyncPacketRepeatCount(u32 version, u16 packet)
{
	if (version >= 4u)
	{
		return (packet >> NATIVE_REPLAY_VSYNC_PACKET_REPEAT_SHIFT) + 1u;
	}
	return 1u;
}

internal s32 NativeReplayScheduler_RecordVSyncLayoutValid(u32 version, const struct NativeReplayFrameRecord *record)
{
	u32 total = 0;
	u32 packetCount;
	u32 preFrameCount;

	if (record == NULL)
	{
		return 0;
	}

	packetCount = NativeReplayScheduler_RecordVSyncPacketCount(version, record);
	preFrameCount = NativeReplayScheduler_RecordPreFrameVSyncPacketCount(version, record);
	if ((packetCount > NATIVE_REPLAY_MAX_VSYNC_PACKETS) || (preFrameCount > packetCount))
	{
		return 0;
	}

	for (u32 i = 0; i < packetCount; i++)
	{
		u32 packetValue = NativeReplayScheduler_VSyncPacketValue(version, record->vblankPackets[i]);
		u32 repeatCount = NativeReplayScheduler_VSyncPacketRepeatCount(version, record->vblankPackets[i]);
		u32 repeatedTotal;

		if ((packetValue == 0) || (repeatCount > NATIVE_REPLAY_VSYNC_PACKET_REPEAT_MAX) ||
		    (packetValue > UINT32_MAX / repeatCount))
		{
			return 0;
		}
		repeatedTotal = packetValue * repeatCount;
		if (total > UINT32_MAX - repeatedTotal)
		{
			return 0;
		}
		total += repeatedTotal;
	}

	return total == record->vblankTotal;
}

internal s32 NativeReplayScheduler_FrameFileOffset(u32 replayFrame, long *offsetOut)
{
	if (offsetOut == NULL)
	{
		return 0;
	}
	if ((u64)replayFrame >
	    (((u64)LONG_MAX - (u64)sizeof(struct NativeReplayFileHeader)) / (u64)sizeof(struct NativeReplayFrameRecord)))
	{
		return 0;
	}

	*offsetOut = (long)sizeof(struct NativeReplayFileHeader) + ((long)replayFrame * (long)sizeof(struct NativeReplayFrameRecord));
	return 1;
}

internal void NativeReplayScheduler_LogHostAddressSample(const char *phase)
{
	const struct PlatformMempackArena *arena = Platform_GetMempackArena();
	const struct GameTracker *gGT = (sdata != NULL) ? sdata->gGT : NULL;
	const struct Driver *driver0 = (gGT != NULL) ? gGT->drivers[0] : NULL;

	Platform_Log("[CTR Replay] %s host-address sample (excluded from canonical digest): sdata=%p gGT=%p driver0=%p mempack=%p\n",
	             phase != NULL ? phase : "unknown", (void *)sdata, (const void *)gGT, (const void *)driver0,
	             (arena != NULL) ? arena->base : NULL);
}

internal const char *NativeReplayScheduler_CheckpointPolicyName(enum NativeReplayCheckpointPolicy policy)
{
	if (policy == NATIVE_REPLAY_CHECKPOINT_POLICY_BOOTSTRAP_ONLY)
	{
		return "bootstrap-only";
	}

	return "rolling";
}

internal const char *NativeReplayScheduler_MetadataStatus(s32 finalMetadata)
{
	if (finalMetadata != 0)
	{
		return "finalized";
	}
	if (s_mode == NATIVE_REPLAY_MODE_ARMED)
	{
		return "armed";
	}
	if (s_mode == NATIVE_REPLAY_MODE_RECORD)
	{
		return "recording";
	}
	if (s_reportCompleted != 0)
	{
		return "finalized";
	}

	return "idle";
}

internal void NativeReplayScheduler_CopyFixedString(char *dst, u32 dstSize, const char *src)
{
	size_t srcLen;

	if ((dst == NULL) || (dstSize == 0))
	{
		return;
	}

	memset(dst, 0, dstSize);
	if (src == NULL)
	{
		return;
	}

	srcLen = strlen(src);
	if (srcLen >= dstSize)
	{
		srcLen = dstSize - 1u;
	}
	memcpy(dst, src, srcLen);
}

internal const char *NativeReplayScheduler_PlatformID(void)
{
#if defined(_WIN32)
	return "win32";
#elif defined(__APPLE__)
	return "macos";
#elif defined(__linux__)
	return "linux";
#else
	return "unknown";
#endif
}

internal u32 NativeReplayScheduler_IdentityChecksum(const struct NativeReplayFileHeader *header)
{
	u32 hash = NATIVE_REPLAY_FNV_OFFSET;

	hash = NativeReplayScheduler_Fnv1aStep(hash, &header->magic, sizeof(header->magic));
	hash = NativeReplayScheduler_Fnv1aStep(hash, &header->version, sizeof(header->version));
	hash = NativeReplayScheduler_Fnv1aStep(hash, &header->headerSize, sizeof(header->headerSize));
	hash = NativeReplayScheduler_Fnv1aStep(hash, &header->frameRecordSize, sizeof(header->frameRecordSize));
	hash = NativeReplayScheduler_Fnv1aStep(hash, &header->checkpointSize, sizeof(header->checkpointSize));
	hash = NativeReplayScheduler_Fnv1aStep(hash, &header->nativeStateSize, sizeof(header->nativeStateSize));
	hash = NativeReplayScheduler_Fnv1aStep(hash, header->buildId, sizeof(header->buildId));
	hash = NativeReplayScheduler_Fnv1aStep(hash, header->platformId, sizeof(header->platformId));
	hash = NativeReplayScheduler_Fnv1aStep(hash, &header->executableFingerprintLow, sizeof(header->executableFingerprintLow));
	hash = NativeReplayScheduler_Fnv1aStep(hash, &header->executableFingerprintHigh, sizeof(header->executableFingerprintHigh));
	return hash;
}

internal void NativeReplayScheduler_InitHeader(struct NativeReplayFileHeader *header)
{
	memset(header, 0, sizeof(*header));
	header->magic = NATIVE_REPLAY_FILE_MAGIC;
	header->version = NATIVE_REPLAY_FILE_VERSION;
	header->headerSize = sizeof(struct NativeReplayFileHeader);
	header->frameRecordSize = sizeof(struct NativeReplayFrameRecord);
	header->checkpointSize = (u32)NativeCheckpoint_GetSize();
	header->nativeStateSize = (u32)NativeState_GetSize();
	header->checkpointPolicy = (u32)s_checkpointPolicy;
	header->executableFingerprintLow = (u32)s_executableFingerprint;
	header->executableFingerprintHigh = (u32)(s_executableFingerprint >> 32);
	NativeReplayScheduler_CopyFixedString(header->buildId, sizeof(header->buildId), CTR_NATIVE_BUILD_ID);
	NativeReplayScheduler_CopyFixedString(header->platformId, sizeof(header->platformId), NativeReplayScheduler_PlatformID());
	header->identityChecksum = NativeReplayScheduler_IdentityChecksum(header);
}

internal s32 NativeReplayScheduler_HeaderLayoutValid(const struct NativeReplayFileHeader *header, u32 minimumVersion, u32 maximumVersion)
{
	if (header == NULL)
	{
		return 0;
	}

	return (header->magic == NATIVE_REPLAY_FILE_MAGIC) && (header->version >= minimumVersion) && (header->version <= maximumVersion) &&
	       (header->headerSize == sizeof(struct NativeReplayFileHeader)) && (header->frameRecordSize == sizeof(struct NativeReplayFrameRecord));
}

internal s32 NativeReplayScheduler_HeaderFormatValid(const struct NativeReplayFileHeader *header)
{
	return NativeReplayScheduler_HeaderLayoutValid(header, NATIVE_REPLAY_FILE_VERSION, NATIVE_REPLAY_FILE_VERSION);
}

internal s32 NativeReplayScheduler_HeaderIdentityValid(const struct NativeReplayFileHeader *header)
{
	struct NativeReplayFileHeader liveHeader;

	if (!NativeReplayScheduler_HeaderFormatValid(header))
	{
		return 0;
	}

	NativeReplayScheduler_InitHeader(&liveHeader);
	return (header->checkpointSize == liveHeader.checkpointSize) && (header->nativeStateSize == liveHeader.nativeStateSize) &&
	       (header->identityChecksum == liveHeader.identityChecksum) && (memcmp(header->buildId, liveHeader.buildId, sizeof(header->buildId)) == 0) &&
	       (memcmp(header->platformId, liveHeader.platformId, sizeof(header->platformId)) == 0) &&
	       (header->executableFingerprintLow == liveHeader.executableFingerprintLow) &&
	       (header->executableFingerprintHigh == liveHeader.executableFingerprintHigh);
}

internal void NativeReplayScheduler_LogHeaderIdentityMismatch(const struct NativeReplayFileHeader *header)
{
	struct NativeReplayFileHeader liveHeader;

	if (header == NULL)
	{
		return;
	}

	NativeReplayScheduler_InitHeader(&liveHeader);
	Platform_Log("[CTR Replay] replay header mismatch: replay(checkpoint=%u nativeState=%u identity=0x%08x executable=%08x%08x build=%.*s platform=%.*s) "
	             "live(checkpoint=%u nativeState=%u identity=0x%08x executable=%08x%08x build=%.*s platform=%.*s)\n",
	             (unsigned int)header->checkpointSize, (unsigned int)header->nativeStateSize, (unsigned int)header->identityChecksum,
	             (unsigned int)header->executableFingerprintHigh, (unsigned int)header->executableFingerprintLow,
	             (int)sizeof(header->buildId), header->buildId, (int)sizeof(header->platformId), header->platformId, (unsigned int)liveHeader.checkpointSize,
	             (unsigned int)liveHeader.nativeStateSize, (unsigned int)liveHeader.identityChecksum,
	             (unsigned int)liveHeader.executableFingerprintHigh, (unsigned int)liveHeader.executableFingerprintLow,
	             (int)sizeof(liveHeader.buildId), liveHeader.buildId,
	             (int)sizeof(liveHeader.platformId), liveHeader.platformId);
}

internal const char *NativeReplayScheduler_ArgValue(int argc, char **argv, const char *arg)
{
	NativeStr8 argText = NativeStr8_FromCString(arg);
	int i;

	for (i = 1; i < argc - 1; i++)
	{
		if (NativeStr8_Equals(NativeStr8_FromCString(argv[i]), argText))
		{
			return argv[i + 1];
		}
	}

	return NULL;
}

internal s32 NativeReplayScheduler_ArgPresent(int argc, char **argv, const char *arg)
{
	NativeStr8 argText = NativeStr8_FromCString(arg);
	int i;

	for (i = 1; i < argc; i++)
	{
		if (NativeStr8_Equals(NativeStr8_FromCString(argv[i]), argText))
		{
			return 1;
		}
	}

	return 0;
}

internal s32 NativeReplayScheduler_ArgMissingValue(int argc, char **argv, const char *arg)
{
	NativeStr8 argText = NativeStr8_FromCString(arg);
	int i;

	for (i = 1; i < argc; i++)
	{
		if (NativeStr8_Equals(NativeStr8_FromCString(argv[i]), argText) &&
		    ((i + 1 >= argc) || NativeStr8_StartsWith(NativeStr8_FromCString(argv[i + 1]), NATIVE_STR8_LIT("--"))))
		{
			return 1;
		}
	}

	return 0;
}

internal s32 NativeReplayScheduler_ParseU32(const char *text, u32 *valueOut)
{
	char *end;
	unsigned long long value;

	if ((text == NULL) || (text[0] == '\0') || (valueOut == NULL))
	{
		return 0;
	}

	errno = 0;
	end = NULL;
	value = strtoull(text, &end, 10);
	if ((errno != 0) || (end == text) || (end == NULL) || (*end != '\0') || (value > UINT32_MAX))
	{
		return 0;
	}

	*valueOut = (u32)value;
	return 1;
}

internal char *NativeReplayScheduler_MakeSiblingPath(const char *path, const char *filename)
{
	NativeStr8 pathText = NativeStr8_FromCString(path);
	NativeStr8 filenameText = NativeStr8_FromCString(filename);
	size_t dirLen;
	size_t separatorIndex;
	char *siblingPath;

	if ((path == NULL) || (filename == NULL))
	{
		return NULL;
	}

	dirLen = NativeStr8_LastIndexOfAny(pathText, '/', '\\', &separatorIndex) ? separatorIndex + 1u : 0u;

	siblingPath = (char *)malloc(dirLen + filenameText.len + 1u);
	if (siblingPath == NULL)
	{
		return NULL;
	}

	memcpy(siblingPath, path, dirLen);
	memcpy(&siblingPath[dirLen], filenameText.ptr, filenameText.len);
	siblingPath[dirLen + filenameText.len] = '\0';
	return siblingPath;
}

internal s32 NativeReplayScheduler_FileExists(const char *path)
{
	FILE *file;

	if (path == NULL)
	{
		return 0;
	}

	file = fopen(path, "rb");
	if (file == NULL)
	{
		return 0;
	}

	fclose(file);
	return 1;
}

internal s32 NativeReplayScheduler_PathExists(const char *path)
{
	struct stat st;

	return (path != NULL) && (stat(path, &st) == 0);
}

internal char *NativeReplayScheduler_DupString(const char *text)
{
	NativeStr8 textView = NativeStr8_FromCString(text);
	char *copy;

	if (text == NULL)
	{
		return NULL;
	}

	copy = (char *)malloc(textView.len + 1u);
	if (copy == NULL)
	{
		return NULL;
	}

	NativeStr8_CopyToCString(copy, textView.len + 1u, textView);
	return copy;
}

internal char *NativeReplayScheduler_JoinPath(const char *left, const char *right)
{
	NativeStr8 leftText = NativeStr8_FromCString(left);
	NativeStr8 rightText = NativeStr8_FromCString(right);
	char *path;

	if ((left == NULL) || (right == NULL))
	{
		return NULL;
	}

	path = (char *)malloc(leftText.len + 1u + rightText.len + 1u);
	if (path == NULL)
	{
		return NULL;
	}

	if (!NativePath_Join(path, leftText.len + 1u + rightText.len + 1u, leftText, rightText))
	{
		free(path);
		return NULL;
	}

	return path;
}

internal s32 NativeReplayScheduler_MakeDir(const char *path)
{
	if ((path == NULL) || (path[0] == '\0'))
	{
		return 1;
	}

#if defined(_WIN32)
	if (_mkdir(path) == 0)
		return 1;
#else
	if (mkdir(path, 0777) == 0)
	{
		return 1;
	}
#endif

	return errno == EEXIST;
}

internal s32 NativeReplayScheduler_CreateDirs(const char *path)
{
	char *copy;
	char *cursor;
	s32 ok = 1;

	if ((path == NULL) || (path[0] == '\0'))
	{
		return 0;
	}

	copy = NativeReplayScheduler_DupString(path);
	if (copy == NULL)
	{
		return 0;
	}

	cursor = copy;
	if (NativePath_IsSeparator(cursor[0]))
	{
		cursor++;
	}
#if defined(_WIN32)
	if ((cursor[0] != '\0') && (cursor[1] == ':') && NativePath_IsSeparator(cursor[2]))
		cursor += 3;
#endif

	while (*cursor != '\0')
	{
		if (NativePath_IsSeparator(*cursor))
		{
			char saved = *cursor;

			*cursor = '\0';
			if ((copy[0] != '\0') && !NativeReplayScheduler_MakeDir(copy))
			{
				ok = 0;
				break;
			}
			*cursor = saved;
		}
		cursor++;
	}

	if ((ok != 0) && !NativeReplayScheduler_MakeDir(copy))
	{
		ok = 0;
	}

	free(copy);
	return ok;
}

internal void NativeReplayScheduler_FreeReportPaths(void)
{
	free(s_reportDir);
	free(s_reportReplayPath);
	free(s_reportCheckpointPath);
	free(s_reportMemcardSeedPath);
	free(s_reportMemcardRecordingPath);
	free(s_reportMetadataPath);
	free(s_reportLogPath);
	s_reportDir = NULL;
	s_reportReplayPath = NULL;
	s_reportCheckpointPath = NULL;
	s_reportMemcardSeedPath = NULL;
	s_reportMemcardRecordingPath = NULL;
	s_reportMetadataPath = NULL;
	s_reportLogPath = NULL;
	s_reportEnabled = 0;
}

internal s32 NativeReplayScheduler_PrepareReportPaths(const char *root)
{
	time_t now;
	struct tm *localTime;
	char dateText[16];
	char runText[32];
	char runCandidate[40];
	char *dateDir = NULL;
	u32 attempt;

	if ((root == NULL) || (root[0] == '\0'))
	{
		return 0;
	}

	now = time(NULL);
	localTime = localtime(&now);
	if (localTime == NULL)
	{
		return 0;
	}

	if ((strftime(dateText, sizeof(dateText), "%Y%m%d", localTime) == 0) || (strftime(runText, sizeof(runText), "ctr-%H%M%S", localTime) == 0))
	{
		return 0;
	}

	NativeReplayScheduler_FreeReportPaths();

	dateDir = NativeReplayScheduler_JoinPath(root, dateText);
	if (dateDir == NULL)
	{
		return 0;
	}
	for (attempt = 0; attempt < 100u; attempt++)
	{
		int written;

		if (attempt == 0)
		{
			written = snprintf(runCandidate, sizeof(runCandidate), "%s", runText);
		}
		else
		{
			written = snprintf(runCandidate, sizeof(runCandidate), "%s-%02u", runText, (unsigned int)attempt);
		}

		if ((written < 0) || ((size_t)written >= sizeof(runCandidate)))
		{
			goto fail;
		}

		s_reportDir = NativeReplayScheduler_JoinPath(dateDir, runCandidate);
		if (s_reportDir == NULL)
		{
			goto fail;
		}
		if (!NativeReplayScheduler_PathExists(s_reportDir))
		{
			break;
		}

		free(s_reportDir);
		s_reportDir = NULL;
	}
	free(dateDir);
	dateDir = NULL;
	if (s_reportDir == NULL)
	{
		goto fail;
	}

	s_reportReplayPath = NativeReplayScheduler_JoinPath(s_reportDir, NATIVE_REPLAY_REPORT_REPLAY_NAME);
	s_reportCheckpointPath = NativeReplayScheduler_JoinPath(s_reportDir, NATIVE_REPLAY_REPORT_CHECKPOINT_NAME);
	s_reportMemcardSeedPath = NativeReplayScheduler_JoinPath(s_reportDir, NATIVE_REPLAY_REPORT_MEMCARD_SEED_NAME);
	s_reportMemcardRecordingPath = NativeReplayScheduler_JoinPath(s_reportDir, NATIVE_REPLAY_RECORDING_MEMCARD_NAME);
	s_reportMetadataPath = NativeReplayScheduler_JoinPath(s_reportDir, NATIVE_REPLAY_REPORT_METADATA_NAME);
	s_reportLogPath = NativeReplayScheduler_JoinPath(s_reportDir, NATIVE_REPLAY_REPORT_LOG_NAME);
	if ((s_reportReplayPath == NULL) || (s_reportCheckpointPath == NULL) || (s_reportMemcardSeedPath == NULL) || (s_reportMemcardRecordingPath == NULL) ||
	    (s_reportMetadataPath == NULL) || (s_reportLogPath == NULL))
	{
		goto fail;
	}

	if (!NativeReplayScheduler_CreateDirs(s_reportDir))
	{
		goto fail;
	}

	if (!Platform_LogSetPath(s_reportLogPath))
	{
		goto fail;
	}

	s_reportEnabled = 1;
	return 1;

fail:
	free(dateDir);
	NativeReplayScheduler_FreeReportPaths();
	return 0;
}

int NativeReplayScheduler_PrepareReportFromArgs(int argc, char **argv)
{
	const s32 recordReport = NativeReplayScheduler_ArgPresent(argc, argv, "--record");
	const s32 recordFromReplay = NativeReplayScheduler_ArgPresent(argc, argv, "--record-from-replay");

	if ((recordReport == 0) && (recordFromReplay == 0))
	{
		return 0;
	}
	if (s_executableFingerprintReady == 0)
	{
		fprintf(stderr, "[CTR Replay] cannot identify the running executable; recording is disabled\n");
		return 1;
	}

	if (NativeReplayScheduler_ArgMissingValue(argc, argv, "--record-from-replay"))
	{
		fprintf(stderr, "[CTR Replay] missing --record-from-replay command value\n");
		return 1;
	}

	if (((recordReport != 0) && (recordFromReplay != 0)) || (NativeReplayScheduler_ArgPresent(argc, argv, "--replay") != 0))
	{
		fprintf(stderr, "[CTR Replay] choose one of --record, --record-from-replay, or --replay\n");
		return 1;
	}

	if (!NativeReplayScheduler_PrepareReportPaths(NATIVE_REPLAY_DEFAULT_REPORT_ROOT))
	{
		fprintf(stderr, "[CTR Replay] failed to prepare report folder under: %s\n", NATIVE_REPLAY_DEFAULT_REPORT_ROOT);
		return 1;
	}

	return 0;
}

internal void NativeReplayScheduler_WriteReportMetadata(s32 finalMetadata)
{
	FILE *file;
	s32 writeFailed;

	if ((s_reportEnabled == 0) || (s_reportMetadataPath == NULL))
	{
		return;
	}

	file = fopen(s_reportMetadataPath, "wb");
	if (file == NULL)
	{
		Platform_Log("[CTR Replay] failed to write report metadata: %s\n", s_reportMetadataPath);
		NativeReplayScheduler_SetFailure();
		return;
	}

	fprintf(file, "ctr_native_report=1\n");
	fprintf(file, "finalized=%d\n", finalMetadata != 0);
	fprintf(file, "recording_status=%s\n", NativeReplayScheduler_MetadataStatus(finalMetadata));
	fprintf(file, "manual_start=%d\n", s_reportManualStart != 0);
	fprintf(file, "start_hotkey=F9\n");
	fprintf(file, "stop_hotkey=F10\n");
	fprintf(file, "ctr_native_version=%s\n", CTR_NATIVE_VERSION);
	fprintf(file, "build_id=%s\n", s_header.buildId);
	fprintf(file, "platform=%s\n", s_header.platformId);
	fprintf(file, "replay_version=%u\n", (unsigned int)s_header.version);
	fprintf(file, "frame_record_size=%u\n", (unsigned int)s_header.frameRecordSize);
	fprintf(file, "checkpoint_size=%u\n", (unsigned int)s_header.checkpointSize);
	fprintf(file, "native_state_size=%u\n", (unsigned int)s_header.nativeStateSize);
	fprintf(file, "checkpoint_mode=%s\n", NativeReplayScheduler_CheckpointPolicyName((enum NativeReplayCheckpointPolicy)s_header.checkpointPolicy));
	fprintf(file, "identity_checksum=0x%08x\n", (unsigned int)s_header.identityChecksum);
	fprintf(file, "executable_fingerprint=%08x%08x\n", (unsigned int)s_header.executableFingerprintHigh,
	        (unsigned int)s_header.executableFingerprintLow);
	fprintf(file, "frame_count=%u\n", (unsigned int)s_header.frameCount);
	fprintf(file, "checkpoint_count=%u\n", (unsigned int)s_header.checkpointCount);
	fprintf(file, "checkpoint_interval_frames=%u\n",
	        s_header.checkpointPolicy == NATIVE_REPLAY_CHECKPOINT_POLICY_ROLLING ? (unsigned int)NATIVE_REPLAY_CHECKPOINT_INTERVAL_FRAMES : 0u);
	fprintf(file, "report_dir=%s\n", s_reportDir != NULL ? s_reportDir : "");
	fprintf(file, "replay_path=%s\n", s_reportReplayPath != NULL ? s_reportReplayPath : "");
	fprintf(file, "checkpoint_path=%s\n", s_reportCheckpointPath != NULL ? s_reportCheckpointPath : "");
	fprintf(file, "memcard_seed_path=%s\n", s_reportMemcardSeedPath != NULL ? s_reportMemcardSeedPath : "");
	fprintf(file, "memcard_recording_path=%s\n", s_reportMemcardRecordingPath != NULL ? s_reportMemcardRecordingPath : "");
	fprintf(file, "log_path=%s\n", Platform_LogGetPath());
	fprintf(file, "input_seed_source=%s\n", s_inputReplayPath != NULL ? s_inputReplayPath : "");
	fprintf(file, "input_seed_frames=%u\n", s_inputFile != NULL ? (unsigned int)s_inputHeader.frameCount : 0u);
	fprintf(file, "playback_command=build/ctr_native --replay \"%s\"\n", s_reportReplayPath != NULL ? s_reportReplayPath : "");
	writeFailed = ferror(file) != 0;
	if (fclose(file) != 0)
	{
		writeFailed = 1;
	}
	if (writeFailed != 0)
	{
		Platform_Log("[CTR Replay] failed to finalize report metadata: %s\n", s_reportMetadataPath);
		NativeReplayScheduler_SetFailure();
	}
}

internal s32 NativeReplayScheduler_MemcardActionBlocksRootSwitch(s16 action)
{
	// NOTE(aalhendi): Native GetInfo is synchronous read-only polling that menus keep queued;
	// only wait on card actions that can touch save contents.
	return (action != 0) && (action != MC_ACTION_GetInfo);
}

internal s32 NativeReplayScheduler_MemcardIdleForRootSwitch(void)
{
	if (sdata == NULL)
	{
		return 1;
	}
	if (sdata->memcard_stage != MC_STAGE_IDLE)
	{
		return 0;
	}
	if (NativeReplayScheduler_MemcardActionBlocksRootSwitch(sdata->frame1_memcardAction) ||
	    NativeReplayScheduler_MemcardActionBlocksRootSwitch(sdata->frame2_memcardAction))
	{
		return 0;
	}

	return 1;
}

internal void NativeReplayScheduler_LogMemcardStartDeferred(void)
{
	if (s_recordStartDeferredLogged != 0)
	{
		return;
	}

	if (sdata != NULL)
	{
		Platform_Log("[CTR Replay] report start waiting for memcard activity to finish (stage=%d frame1=%d frame2=%d)\n", sdata->memcard_stage,
		             sdata->frame1_memcardAction, sdata->frame2_memcardAction);
	}
	else
	{
		Platform_Log("[CTR Replay] report start waiting for memcard activity to finish\n");
	}
	s_recordStartDeferredLogged = 1;
}

internal void NativeReplayScheduler_ResetMemcardSandbox(void)
{
	if (s_memcardSandboxActive != 0)
	{
		NativeMemcard_ClearRoot();
		s_memcardSandboxActive = 0;
	}

	if (s_playbackMemcardPath != NULL)
	{
		if (NativeMemcard_RemoveRoot(s_playbackMemcardPath) != NATIVE_MEMCARD_OK)
		{
			Platform_Log("[CTR Replay] failed to remove playback memcard sandbox: %s\n", s_playbackMemcardPath);
			NativeReplayScheduler_SetFailure();
		}

		free(s_playbackMemcardPath);
		s_playbackMemcardPath = NULL;
	}
}

internal s32 NativeReplayScheduler_ActivateRecordMemcardSandbox(const char *sourceSeedPath)
{
	enum NativeMemcardResult result;

	if ((s_reportMemcardSeedPath == NULL) || (s_reportMemcardRecordingPath == NULL) || !NativeReplayScheduler_MemcardIdleForRootSwitch())
	{
		return 0;
	}

	if (sourceSeedPath != NULL)
	{
		result = NativeMemcard_CloneRoot(sourceSeedPath, s_reportMemcardSeedPath);
	}
	else
	{
		result = NativeMemcard_CloneCurrentRoot(s_reportMemcardSeedPath);
	}
	if (result != NATIVE_MEMCARD_OK)
	{
		Platform_Log("[CTR Replay] failed to clone memcard seed into report: source=%s destination=%s\n",
		             sourceSeedPath != NULL ? sourceSeedPath : "current-root", s_reportMemcardSeedPath);
		return 0;
	}

	result = NativeMemcard_CloneRoot(s_reportMemcardSeedPath, s_reportMemcardRecordingPath);
	if (result != NATIVE_MEMCARD_OK)
	{
		Platform_Log("[CTR Replay] failed to prepare recording memcard sandbox: %s\n", s_reportMemcardRecordingPath);
		return 0;
	}

	result = NativeMemcard_SetRoot(s_reportMemcardRecordingPath);
	if (result != NATIVE_MEMCARD_OK)
	{
		Platform_Log("[CTR Replay] failed to enter recording memcard sandbox: %s\n", s_reportMemcardRecordingPath);
		return 0;
	}

	s_memcardSandboxActive = 1;
	Platform_Log("[CTR Replay] memcard seed: %s\n", s_reportMemcardSeedPath);
	Platform_Log("[CTR Replay] recording memcard sandbox: %s\n", s_reportMemcardRecordingPath);
	return 1;
}

internal s32 NativeReplayScheduler_ActivatePlaybackMemcardSandbox(const char *replayPath)
{
	char *sourcePath;
	enum NativeMemcardResult result;

	sourcePath = NativeReplayScheduler_MakeSiblingPath(replayPath, NATIVE_REPLAY_REPORT_MEMCARD_SEED_NAME);
	s_playbackMemcardPath = NativeReplayScheduler_MakeSiblingPath(replayPath, NATIVE_REPLAY_PLAYBACK_MEMCARD_NAME);
	if ((sourcePath == NULL) || (s_playbackMemcardPath == NULL))
	{
		free(sourcePath);
		free(s_playbackMemcardPath);
		s_playbackMemcardPath = NULL;
		return 0;
	}

	if (!NativeReplayScheduler_PathExists(sourcePath))
	{
		Platform_Log("[CTR Replay] replay is missing memcard seed: %s\n", sourcePath);
		free(sourcePath);
		return 0;
	}

	// TODO(aalhendi): Per-checkpoint resume needs checkpoint-indexed memcard
	// snapshots or a deterministic memcard mutation log. This seed only
	// guarantees frame-0 playback starts from the recorded card state.
	result = NativeMemcard_CloneRoot(sourcePath, s_playbackMemcardPath);
	if (result != NATIVE_MEMCARD_OK)
	{
		Platform_Log("[CTR Replay] failed to prepare playback memcard sandbox: %s\n", s_playbackMemcardPath);
		free(sourcePath);
		return 0;
	}

	result = NativeMemcard_SetRoot(s_playbackMemcardPath);
	if (result != NATIVE_MEMCARD_OK)
	{
		Platform_Log("[CTR Replay] failed to enter playback memcard sandbox: %s\n", s_playbackMemcardPath);
		free(sourcePath);
		return 0;
	}

	free(sourcePath);
	s_memcardSandboxActive = 1;
	Platform_Log("[CTR Replay] playback memcard sandbox: %s\n", s_playbackMemcardPath);
	return 1;
}

internal s32 NativeReplayScheduler_WriteHeader(void)
{
	long oldPos;

	if (s_file == NULL)
	{
		return 0;
	}

	oldPos = ftell(s_file);
	if (oldPos < 0)
	{
		return 0;
	}

	if (fseek(s_file, 0, SEEK_SET) != 0)
	{
		return 0;
	}

	if (fwrite(&s_header, sizeof(s_header), 1, s_file) != 1)
	{
		return 0;
	}

	if (fseek(s_file, oldPos, SEEK_SET) != 0)
	{
		return 0;
	}

	return fflush(s_file) == 0;
}

internal void NativeReplayScheduler_CloseCheckpointFile(void)
{
	if (s_checkpointWriterOpen != 0)
	{
		if (!NativeCheckpointFile_EndWrite(&s_checkpointWriter))
		{
			Platform_Log("[CTR State] failed to finalize rolling checkpoints\n");
			NativeReplayScheduler_SetFailure();
		}
		s_checkpointWriterOpen = 0;
	}

	free(s_checkpointPayload);
	s_checkpointPayload = NULL;
	s_checkpointPayloadSize = 0;

	free(s_checkpointPath);
	s_checkpointPath = NULL;

	s_checkpointIndex = 0;
	s_nextCheckpointFrame = 0;
	s_restoreBootstrapCheckpoint = 0;
	s_restoreCheckpointIndex = 0;
	s_frameTimingConsumed = 0;
	NativeReplayScheduler_ResetVSyncPackets();
}

internal void NativeReplayScheduler_CloseInputReplay(void)
{
	if (s_inputFile != NULL)
	{
		if ((s_exitStatus == 0) && (s_replayFrame < s_inputHeader.frameCount))
		{
			Platform_Log("[CTR Replay] replay-seeded recording stopped early at frame %u of %u\n", s_replayFrame, s_inputHeader.frameCount);
			NativeReplayScheduler_SetFailure();
		}
		if (fclose(s_inputFile) != 0)
		{
			Platform_Log("[CTR Replay] failed to close replay seed input\n");
			NativeReplayScheduler_SetFailure();
		}
	}

	s_inputFile = NULL;
	memset(&s_inputHeader, 0, sizeof(s_inputHeader));
	memset(&s_inputRecord, 0, sizeof(s_inputRecord));
	s_inputVBlankPacketCursor = 0;
	s_inputVBlankPacketRepeatCursor = 0;
	s_inputRecordReady = 0;
	s_inputCompleteTiming = 0;
	s_inputVBlankPlaybackMismatch = 0;
	free(s_inputReplayPath);
	s_inputReplayPath = NULL;
}

internal void NativeReplayScheduler_CloseFiles(void)
{
	NativeReplayScheduler_CheckPlaybackComplete();

	if (s_file == NULL)
	{
		NativeReplayScheduler_CloseInputReplay();
		NativeReplayScheduler_CloseCheckpointFile();
		NativeAudio_SetDeterministicRenderMode(0);
		s_mode = NATIVE_REPLAY_MODE_NONE;
		s_stopRequested = 0;
		return;
	}

	if (s_mode == NATIVE_REPLAY_MODE_RECORD)
	{
		s_header.checkpointCount = s_checkpointIndex;
		if (!NativeReplayScheduler_WriteHeader())
		{
			Platform_Log("[CTR Replay] failed to finalize replay header\n");
			NativeReplayScheduler_SetFailure();
		}
		NativeReplayScheduler_WriteReportMetadata(1);
	}

	if (fclose(s_file) != 0)
	{
		Platform_Log("[CTR Replay] failed to close replay file\n");
		NativeReplayScheduler_SetFailure();
	}
	s_file = NULL;
	NativeReplayScheduler_CloseInputReplay();
	NativeReplayScheduler_CloseCheckpointFile();
	NativeAudio_SetDeterministicRenderMode(0);
	if (s_reportEnabled != 0)
	{
		s_reportCompleted = 1;
	}
	s_stopRequested = 0;
	s_mode = NATIVE_REPLAY_MODE_NONE;
}

internal s32 NativeReplayScheduler_OpenCheckpointRecord(const char *checkpointPath)
{
	s_checkpointPath = NativeReplayScheduler_DupString(checkpointPath);
	if (s_checkpointPath == NULL)
	{
		Platform_Log("[CTR State] failed to build checkpoint path\n");
		return 0;
	}

	s_checkpointPayloadSize = NativeCheckpoint_GetSize();
	if (s_checkpointPayloadSize <= 0)
	{
		Platform_Log("[CTR State] invalid checkpoint size: %d\n", s_checkpointPayloadSize);
		return 0;
	}

	s_checkpointPayload = (u8 *)malloc((size_t)s_checkpointPayloadSize);
	if (s_checkpointPayload == NULL)
	{
		Platform_Log("[CTR State] failed to allocate checkpoint buffer: %d bytes\n", s_checkpointPayloadSize);
		return 0;
	}

	if (!NativeCheckpointFile_BeginWrite(&s_checkpointWriter, s_checkpointPath))
	{
		Platform_Log("[CTR State] failed to open rolling checkpoints: %s\n", s_checkpointPath);
		return 0;
	}

	s_checkpointWriterOpen = 1;
	s_checkpointIndex = 0;
	s_nextCheckpointFrame = 0;
	if (s_checkpointPolicy == NATIVE_REPLAY_CHECKPOINT_POLICY_BOOTSTRAP_ONLY)
	{
		Platform_Log("[CTR State] recording bootstrap checkpoint only: %s\n", s_checkpointPath);
	}
	else
	{
		Platform_Log("[CTR State] recording rolling checkpoints: %s interval=%u frames\n", s_checkpointPath, NATIVE_REPLAY_CHECKPOINT_INTERVAL_FRAMES);
	}
	return 1;
}

internal s32 NativeReplayScheduler_WriteCheckpointIfDue(void)
{
	struct NativeCheckpointFileRecordInfo info;

	if (s_checkpointWriterOpen == 0)
	{
		return 1;
	}
	if ((s_checkpointPolicy == NATIVE_REPLAY_CHECKPOINT_POLICY_BOOTSTRAP_ONLY) && (s_checkpointIndex != 0))
	{
		return 1;
	}
	if ((s_replayFrame != 0) && (s_replayFrame < s_nextCheckpointFrame))
	{
		return 1;
	}

	if (!NativeCheckpoint_Capture(s_checkpointPayload, s_checkpointPayloadSize))
	{
		Platform_Log("[CTR State] failed to capture checkpoint #%u at replay frame %u\n", s_checkpointIndex, s_replayFrame);
		return 0;
	}

	if (!NativeCheckpointFile_AppendRecord(&s_checkpointWriter, s_checkpointPayload, s_checkpointPayloadSize, s_checkpointIndex, s_replayFrame, &info))
	{
		Platform_Log("[CTR State] failed to write checkpoint #%u at replay frame %u\n", s_checkpointIndex, s_replayFrame);
		return 0;
	}

	Platform_Log("[CTR State] checkpoint #%u replayFrame=%u checksum=0x%08x\n", info.checkpointIndex, info.replayFrame, info.checksum);
	if (info.checkpointIndex == 0)
	{
		NativeReplayScheduler_LogHostAddressSample("record");
	}
	s_checkpointIndex++;
	s_header.checkpointCount = s_checkpointIndex;
	if (!NativeReplayScheduler_WriteHeader())
	{
		Platform_Log("[CTR Replay] failed to update replay header checkpoint count\n");
		return 0;
	}
	s_nextCheckpointFrame = s_replayFrame + NATIVE_REPLAY_CHECKPOINT_INTERVAL_FRAMES;
	NativeReplayScheduler_WriteReportMetadata(0);
	return 1;
}

internal s32 NativeReplayScheduler_PrepareBootstrapCheckpoint(const char *replayPath, u32 checkpointIndex)
{
	char *checkpointPath = NativeReplayScheduler_MakeSiblingPath(replayPath, NATIVE_REPLAY_REPORT_CHECKPOINT_NAME);
	int recordCount = 0;

	if (checkpointPath == NULL)
	{
		Platform_Log("[CTR State] failed to build checkpoint path\n");
		return 0;
	}

	if (!NativeReplayScheduler_FileExists(checkpointPath))
	{
		Platform_Log("[CTR State] missing rolling checkpoints for replay: %s\n", checkpointPath);
		free(checkpointPath);
		return 0;
	}

	if (!NativeCheckpointFile_Validate(checkpointPath, NULL, 0, &recordCount))
	{
		Platform_Log("[CTR State] invalid rolling checkpoints: %s\n", checkpointPath);
		free(checkpointPath);
		return 0;
	}

	Platform_Log("[CTR State] validated replay checkpoints: %s records=%d\n", checkpointPath, recordCount);
	if (recordCount <= 0)
	{
		Platform_Log("[CTR State] replay checkpoints are empty: %s\n", checkpointPath);
		free(checkpointPath);
		return 0;
	}
	if ((u32)recordCount != s_header.checkpointCount)
	{
		Platform_Log("[CTR State] checkpoint count mismatch: replay=%u state=%d\n", s_header.checkpointCount, recordCount);
		free(checkpointPath);
		return 0;
	}
	if (checkpointIndex >= (u32)recordCount)
	{
		Platform_Log("[CTR State] requested checkpoint %u is outside replay checkpoint count %d\n", checkpointIndex, recordCount);
		free(checkpointPath);
		return 0;
	}

	s_checkpointPath = checkpointPath;
	s_restoreCheckpointIndex = checkpointIndex;
	s_restoreBootstrapCheckpoint = 1;
	return 1;
}

internal s32 NativeReplayScheduler_RestoreBootstrapCheckpoint(void)
{
	struct NativeCheckpointFileRecordInfo info;
	u8 *payload;
	u32 recordedChecksum;
	u32 recapturedChecksum;
	int payloadSize;
	long replayOffset;
	s32 ok = 0;

	if (s_restoreBootstrapCheckpoint == 0)
	{
		return 1;
	}

	payloadSize = NativeCheckpoint_GetSize();
	if (payloadSize <= 0)
	{
		Platform_Log("[CTR State] invalid checkpoint size: %d\n", payloadSize);
		return 0;
	}

	payload = (u8 *)malloc((size_t)payloadSize);
	if (payload == NULL)
	{
		Platform_Log("[CTR State] failed to allocate checkpoint restore buffer: %d bytes\n", payloadSize);
		return 0;
	}

	if (!NativeCheckpointFile_ReadRecord(s_checkpointPath, s_restoreCheckpointIndex, payload, payloadSize, &info))
	{
		Platform_Log("[CTR State] failed to read replay checkpoint %u: %s\n", s_restoreCheckpointIndex, s_checkpointPath);
		goto cleanup;
	}
	if ((info.checkpointIndex != s_restoreCheckpointIndex) || (info.replayFrame >= s_header.frameCount))
	{
		Platform_Log("[CTR State] replay checkpoint %u has invalid frame mapping %u for %u frames\n", info.checkpointIndex, info.replayFrame,
		             s_header.frameCount);
		goto cleanup;
	}
	if (!NativeReplayScheduler_FrameFileOffset(info.replayFrame, &replayOffset))
	{
		Platform_Log("[CTR State] replay checkpoint %u frame offset exceeds host file range\n", info.checkpointIndex);
		goto cleanup;
	}
	if (fseek(s_file, replayOffset, SEEK_SET) != 0)
	{
		Platform_Log("[CTR State] failed to seek replay input to checkpoint frame %u\n", info.replayFrame);
		goto cleanup;
	}
	recordedChecksum = NativeReplayScheduler_Fnv1a(payload, (u32)payloadSize);
	if (recordedChecksum != info.checksum)
	{
		Platform_Log("[CTR State] replay checkpoint checksum changed after validation: file=0x%08x memory=0x%08x\n", info.checksum,
		             recordedChecksum);
		goto cleanup;
	}
	if (!NativeCheckpoint_Restore(payload, payloadSize))
	{
		Platform_Log("[CTR State] failed to restore replay checkpoint %u\n", info.checkpointIndex);
		goto cleanup;
	}
	if (!NativeCheckpoint_Capture(payload, payloadSize))
	{
		Platform_Log("[CTR State] failed to recapture restored replay checkpoint %u\n", info.checkpointIndex);
		goto cleanup;
	}
	recapturedChecksum = NativeReplayScheduler_Fnv1a(payload, (u32)payloadSize);

	s_replayFrame = info.replayFrame;
	Platform_Log("[CTR State] restored replay checkpoint #%u replayFrame=%u checksum=0x%08x\n", info.checkpointIndex, info.replayFrame, info.checksum);
	Platform_Log("[CTR State] raw checkpoint comparison (diagnostic only): recorded=0x%08x restored-process=0x%08x equal=%s\n", recordedChecksum,
	             recapturedChecksum, recordedChecksum == recapturedChecksum ? "yes" : "no");
	NativeReplayScheduler_LogHostAddressSample("playback");
	s_restoreBootstrapCheckpoint = 0;
	ok = 1;

cleanup:
	free(payload);
	return ok;
}

internal s32 NativeReplayScheduler_OpenRecord(const char *replayPath, const char *checkpointPath, const char *sourceSeedPath)
{
	NativeReplayScheduler_InitHeader(&s_header);
	if (!NativeReplayScheduler_ActivateRecordMemcardSandbox(sourceSeedPath))
	{
		return 0;
	}

	s_file = fopen(replayPath, "wb+");
	if (s_file == NULL)
	{
		Platform_Log("[CTR Replay] failed to open replay for record: %s\n", replayPath);
		NativeReplayScheduler_ResetMemcardSandbox();
		return 0;
	}

	if (fwrite(&s_header, sizeof(s_header), 1, s_file) != 1)
	{
		Platform_Log("[CTR Replay] failed to write replay header: %s\n", replayPath);
		NativeReplayScheduler_CloseFiles();
		NativeReplayScheduler_ResetMemcardSandbox();
		return 0;
	}
	fflush(s_file);

	s_mode = NATIVE_REPLAY_MODE_RECORD;
	NativeAudio_SetDeterministicRenderMode(1);
	if (!NativeReplayScheduler_OpenCheckpointRecord(checkpointPath))
	{
		NativeReplayScheduler_CloseFiles();
		NativeReplayScheduler_ResetMemcardSandbox();
		remove(replayPath);
		return 0;
	}

	NativeReplayScheduler_WriteReportMetadata(0);
	Platform_Log("[CTR Replay] recording input replay: %s\n", replayPath);
	return 1;
}

internal s32 NativeReplayScheduler_ReadInputRecord(u32 replayFrame)
{
	u32 inputChecksum;

	if ((s_inputFile == NULL) || (s_inputRecordReady != 0))
	{
		return (s_inputRecordReady != 0) && (s_inputRecord.replayFrame == replayFrame);
	}
	if (fread(&s_inputRecord, sizeof(s_inputRecord), 1, s_inputFile) != 1)
	{
		Platform_Log("[CTR Replay] failed to read replay seed frame %u\n", replayFrame);
		return 0;
	}

	inputChecksum = NativeReplayScheduler_RecordChecksum(&s_inputRecord);
	if ((s_inputRecord.magic != NATIVE_REPLAY_FRAME_MAGIC) || (s_inputRecord.replayFrame != replayFrame) ||
	    (inputChecksum != s_inputRecord.recordChecksum) ||
	    (NativeReplayScheduler_PadChecksum(s_inputRecord.pads) != s_inputRecord.padChecksum) ||
	    !NativeReplayScheduler_RecordVSyncLayoutValid(s_inputHeader.version, &s_inputRecord))
	{
		Platform_Log("[CTR Replay] corrupt replay seed frame %u\n", replayFrame);
		return 0;
	}

	s_inputVBlankPacketCursor = 0;
	s_inputVBlankPacketRepeatCursor = 0;
	s_inputRecordReady = 1;
	s_inputVBlankPlaybackMismatch = 0;
	return 1;
}

internal s32 NativeReplayScheduler_OpenInputReplay(const char *path)
{
	s_inputFile = fopen(path, "rb");
	if (s_inputFile == NULL)
	{
		Platform_Log("[CTR Replay] failed to open replay seed input: %s\n", path);
		return 0;
	}

	if (fread(&s_inputHeader, sizeof(s_inputHeader), 1, s_inputFile) != 1)
	{
		Platform_Log("[CTR Replay] failed to read replay seed header: %s\n", path);
		NativeReplayScheduler_CloseInputReplay();
		return 0;
	}
	if (!NativeReplayScheduler_HeaderLayoutValid(&s_inputHeader, NATIVE_REPLAY_INPUT_MIN_VERSION, NATIVE_REPLAY_FILE_VERSION) ||
	    (s_inputHeader.frameCount == 0))
	{
		Platform_Log("[CTR Replay] invalid replay seed input: %s\n", path);
		NativeReplayScheduler_CloseInputReplay();
		return 0;
	}

	s_inputReplayPath = NativeReplayScheduler_DupString(path);
	if (s_inputReplayPath == NULL)
	{
		Platform_Log("[CTR Replay] failed to retain replay seed path\n");
		NativeReplayScheduler_CloseInputReplay();
		return 0;
	}

	s_inputCompleteTiming = s_inputHeader.version >= 4u;
	if ((s_inputCompleteTiming != 0) && !NativeReplayScheduler_ReadInputRecord(0))
	{
		NativeReplayScheduler_CloseInputReplay();
		return 0;
	}

	return 1;
}

internal s32 NativeReplayScheduler_OpenRecordFromReplay(const char *sourceReplayPath)
{
	char *sourceSeedPath;
	s32 opened;

	if (!NativeReplayScheduler_OpenInputReplay(sourceReplayPath))
	{
		return 0;
	}

	sourceSeedPath = NativeReplayScheduler_MakeSiblingPath(sourceReplayPath, NATIVE_REPLAY_REPORT_MEMCARD_SEED_NAME);
	if ((sourceSeedPath == NULL) || !NativeReplayScheduler_PathExists(sourceSeedPath))
	{
		Platform_Log("[CTR Replay] replay seed is missing memcard seed: %s\n", sourceSeedPath != NULL ? sourceSeedPath : "(null)");
		free(sourceSeedPath);
		NativeReplayScheduler_CloseInputReplay();
		return 0;
	}

	opened = NativeReplayScheduler_OpenRecord(s_reportReplayPath, s_reportCheckpointPath, sourceSeedPath);
	free(sourceSeedPath);
	if (!opened)
	{
		NativeReplayScheduler_CloseInputReplay();
		NativeReplayScheduler_ResetMemcardSandbox();
		return 0;
	}

	Platform_Log("[CTR Replay] replay-seeded recording: source=%s frames=%u destination=%s\n", sourceReplayPath, s_inputHeader.frameCount,
	             s_reportReplayPath);
	Platform_Log("[CTR Replay] replay seed timing boundary: version=%u complete-vsync=%s\n", s_inputHeader.version,
	             s_inputCompleteTiming != 0 ? "yes" : "no (coverage automation only)");
	return 1;
}

internal s32 NativeReplayScheduler_ArmReport(void)
{
	if ((s_reportEnabled == 0) || (s_reportReplayPath == NULL))
	{
		return 0;
	}

	NativeReplayScheduler_ResetSessionState();
	NativeReplayScheduler_InitHeader(&s_header);
	s_mode = NATIVE_REPLAY_MODE_ARMED;
	s_reportManualStart = 1;
	s_reportCompleted = 0;
	NativeReplayScheduler_WriteReportMetadata(0);
	Platform_Log("[CTR Replay] report armed: press F9 to start recording, F10 to stop\n");
	return 1;
}

internal s32 NativeReplayScheduler_StartReportRecording(void)
{
	if ((s_reportEnabled == 0) || (s_reportReplayPath == NULL))
	{
		return 0;
	}
	if (!NativeReplayScheduler_MemcardIdleForRootSwitch())
	{
		NativeReplayScheduler_LogMemcardStartDeferred();
		return 0;
	}
	if (s_reportCompleted != 0)
	{
		Platform_Log("[CTR Replay] report already finalized: %s\n", s_reportDir != NULL ? s_reportDir : "");
		return 0;
	}

	NativeReplayScheduler_ResetSessionState();
	if (!NativeReplayScheduler_OpenRecord(s_reportReplayPath, s_reportCheckpointPath, NULL))
	{
		return 0;
	}

	Platform_Log("[CTR Replay] report recording started\n");
	return 1;
}

internal s32 NativeReplayScheduler_OpenPlayback(const char *path, s32 bypassHeaderIdentity, u32 checkpointIndex)
{
	s_file = fopen(path, "rb");
	if (s_file == NULL)
	{
		Platform_Log("[CTR Replay] failed to open replay for playback: %s\n", path);
		return 0;
	}

	if (fread(&s_header, sizeof(s_header), 1, s_file) != 1)
	{
		Platform_Log("[CTR Replay] failed to read replay header: %s\n", path);
		NativeReplayScheduler_CloseFiles();
		return 0;
	}

	if (!NativeReplayScheduler_HeaderFormatValid(&s_header))
	{
		Platform_Log("[CTR Replay] invalid replay file header: %s\n", path);
		NativeReplayScheduler_CloseFiles();
		return 0;
	}
	if (!NativeReplayScheduler_HeaderIdentityValid(&s_header))
	{
		NativeReplayScheduler_LogHeaderIdentityMismatch(&s_header);
		if (bypassHeaderIdentity == 0)
		{
			Platform_Log("[CTR Replay] invalid replay header: %s\n", path);
			NativeReplayScheduler_CloseFiles();
			return 0;
		}
		Platform_Log("[CTR Replay] bypassing replay header identity mismatch: %s\n", path);
	}

	if (!NativeReplayScheduler_PrepareBootstrapCheckpoint(path, checkpointIndex))
	{
		NativeReplayScheduler_CloseFiles();
		return 0;
	}
	if (!NativeReplayScheduler_ActivatePlaybackMemcardSandbox(path))
	{
		NativeReplayScheduler_CloseFiles();
		NativeReplayScheduler_ResetMemcardSandbox();
		return 0;
	}

	s_mode = NATIVE_REPLAY_MODE_PLAYBACK;
	NativeAudio_SetDeterministicRenderMode(1);
	Platform_Log("[CTR Replay] playing input replay: %s frames=%u\n", path, s_header.frameCount);
	if (s_testPerturbEnabled != 0)
	{
		Platform_Log("[CTR Replay] test perturbation armed for driver[0].posCurr.x at replay frame %u\n", s_testPerturbFrame);
	}
	return 1;
}

internal void NativeReplayScheduler_LogFrameInfo(const char *prefix, const struct NativeReplaySchedulerFrameInfo *info)
{
	Platform_Log("[CTR Replay] %s vsync=%d frameCounter=%d timer=%d levFrames=%d elapsedMS=%d msLEV=%d eventMS=%d state=%d loading=%d level=%d "
	             "rng=(mix=0x%08x audio=0x%08x dead=0x%08x,0x%08x adv=0x%08x,0x%08x)\n",
	             prefix, info->frameTimer, info->frameCounter, info->timer, info->framesInThisLEV, info->elapsedTimeMS, info->msInThisLEV,
	             info->elapsedEventTime, info->mainGameState, info->loadingStage, info->levelID, info->mixRandomNumber, info->audioRNG, info->deadcoed0,
	             info->deadcoed1, info->advRng0, info->advRng1);
	Platform_Log("[CTR Replay] %s digest root=%08x%08x timing=%08x%08x rng=%08x%08x drivers=%08x%08x world=%08x%08x allocation=%08x%08x\n",
	             prefix, (u32)(info->stateDigest.root >> 32), (u32)info->stateDigest.root, (u32)(info->stateDigest.timing >> 32),
	             (u32)info->stateDigest.timing, (u32)(info->stateDigest.rng >> 32), (u32)info->stateDigest.rng,
	             (u32)(info->stateDigest.drivers >> 32), (u32)info->stateDigest.drivers, (u32)(info->stateDigest.world >> 32),
	             (u32)info->stateDigest.world, (u32)(info->stateDigest.allocation >> 32), (u32)info->stateDigest.allocation);
}

internal s32 NativeReplayScheduler_FrameInfoMatches(const struct NativeReplaySchedulerFrameInfo *expected, const struct NativeReplaySchedulerFrameInfo *live)
{
	return (expected->frameTimer == live->frameTimer) && (expected->frameCounter == live->frameCounter) && (expected->timer == live->timer) &&
	       (expected->framesInThisLEV == live->framesInThisLEV) && (expected->elapsedTimeMS == live->elapsedTimeMS) &&
	       (expected->msInThisLEV == live->msInThisLEV) && (expected->elapsedEventTime == live->elapsedEventTime) &&
	       (expected->mainGameState == live->mainGameState) && (expected->loadingStage == live->loadingStage) && (expected->levelID == live->levelID) &&
	       (expected->mixRandomNumber == live->mixRandomNumber) && (expected->audioRNG == live->audioRNG) && (expected->deadcoed0 == live->deadcoed0) &&
	       (expected->deadcoed1 == live->deadcoed1) && (expected->advRng0 == live->advRng0) && (expected->advRng1 == live->advRng1) &&
	       (NativeStateDigest_DifferenceMask(&expected->stateDigest, &live->stateDigest) == 0) &&
	       (expected->stateDigest.root == live->stateDigest.root);
}

internal s32 NativeReplayScheduler_VSyncInfoMatches(const struct NativeReplayFrameRecord *expected)
{
	return (s_vblankPlaybackMismatch == 0) && (s_frameVBlankPacketRepeatCursor == 0) &&
	       (expected->vblankTotal == s_frameVBlankTotal) &&
	       (NativeReplayScheduler_RecordVSyncPacketCount(s_header.version, expected) == s_frameVBlankPacketCount);
}

internal void NativeReplayScheduler_LogAllocationPool(const char *name, const struct JitPool *pool)
{
	Platform_Log("[CTR Replay] live allocation pool %s: free=%d taken=%d max=%d itemSize=%u poolSize=%d\n", name, pool->free.count,
	             pool->taken.count, pool->maxItems, pool->itemSize, pool->poolSize);
}

internal void NativeReplayScheduler_LogTimingState(void)
{
	const struct GameTracker *gGT = (sdata != NULL) ? sdata->gGT : NULL;

	if (gGT == NULL)
	{
		Platform_Log("[CTR Replay] live timing state: no game tracker\n");
		return;
	}

	Platform_Log("[CTR Replay] live timing state: frameTimer=%d notPaused=%d timer=%d levFrames=%d msLEV=%d elapsedMS=%d eventMS=%d "
	             "traffic=%d frozen=%d originalEvent=%d clockStall=%d frameCounter=%d\n",
	             gGT->frameTimer_VsyncCallback, gGT->frameTimer_notPaused, gGT->timer, gGT->framesInThisLEV, gGT->msInThisLEV,
	             gGT->elapsedTimeMS, gGT->elapsedEventTime, gGT->trafficLightsTimer, gGT->frozenTimeRemaining, gGT->originalEventTime,
	             gGT->clockDurationStall, sdata->frameCounter);
}

internal void NativeReplayScheduler_LogAllocationState(void)
{
	const struct GameTracker *gGT = (sdata != NULL) ? sdata->gGT : NULL;

	if (gGT == NULL)
	{
		Platform_Log("[CTR Replay] live allocation state: no game tracker\n");
		return;
	}

	NativeReplayScheduler_LogAllocationPool("thread", &gGT->JitPools.thread);
	NativeReplayScheduler_LogAllocationPool("instance", &gGT->JitPools.instance);
	NativeReplayScheduler_LogAllocationPool("smallStack", &gGT->JitPools.smallStack);
	NativeReplayScheduler_LogAllocationPool("mediumStack", &gGT->JitPools.mediumStack);
	NativeReplayScheduler_LogAllocationPool("largeStack", &gGT->JitPools.largeStack);
	NativeReplayScheduler_LogAllocationPool("particle", &gGT->JitPools.particle);
	NativeReplayScheduler_LogAllocationPool("oscillator", &gGT->JitPools.oscillator);
	NativeReplayScheduler_LogAllocationPool("rain", &gGT->JitPools.rain);
	Platform_Log("[CTR Replay] live allocation activeMempack=%d\n", gGT->activeMempackIndex);
	for (u32 i = 0; i < len(sdata->mempack); i++)
	{
		const struct Mempack *mempack = &sdata->mempack[i];

		Platform_Log("[CTR Replay] live allocation mempack[%u]: packSize=%d start=%p last=%p allocatorEnd=%p memoryEnd=%p first=%p "
		             "previous=%d bookmarks=%d\n",
		             i, mempack->packSize, mempack->start, mempack->lastFreeByte, mempack->endOfAllocator, mempack->endOfMemory,
		             mempack->firstFreeByte, mempack->sizeOfPrevAllocation, mempack->numBookmarks);
	}
}

internal void NativeReplayScheduler_ReportDivergence(const struct NativeReplayFrameRecord *expected, const struct NativeReplaySchedulerFrameInfo *live,
                                                     u32 livePadChecksum)
{
	u32 stateDifference;

	if (s_divergenceLogged != 0)
	{
		return;
	}

	s_divergenceLogged = 1;
	s_exitStatus = NATIVE_REPLAY_EXIT_DIVERGENCE;
	Platform_Log("[CTR Replay] divergence at replay frame %u\n", expected->replayFrame);
	NativeReplayScheduler_LogFrameInfo("expected", &expected->endInfo);
	NativeReplayScheduler_LogFrameInfo("live    ", live);
	stateDifference = NativeStateDigest_DifferenceMask(&expected->endInfo.stateDigest, &live->stateDigest);
	if ((stateDifference != 0) || (expected->endInfo.stateDigest.root != live->stateDigest.root))
	{
		Platform_Log("[CTR Replay] first canonical state difference: %s mask=0x%08x\n",
		             NativeStateDigest_FirstDifferenceName(stateDifference), stateDifference);
	}
	if ((stateDifference & NATIVE_STATE_DIGEST_COMPONENT_ALLOCATION) != 0)
	{
		NativeReplayScheduler_LogAllocationState();
	}
	if ((stateDifference & NATIVE_STATE_DIGEST_COMPONENT_TIMING) != 0)
	{
		NativeReplayScheduler_LogTimingState();
	}
	Platform_Log("[CTR Replay] expected padChecksum=0x%08x live padChecksum=0x%08x\n", expected->padChecksum, livePadChecksum);
	Platform_Log("[CTR Replay] expected vblankPackets=%u preFrame=%u vblankSteps=%u live vblankPackets=%u preFrame=%u vblankSteps=%u\n",
	             NativeReplayScheduler_RecordVSyncPacketCount(s_header.version, expected),
	             NativeReplayScheduler_RecordPreFrameVSyncPacketCount(s_header.version, expected), expected->vblankTotal,
	             s_frameVBlankPacketCount, s_framePreVBlankPacketCount, s_frameVBlankTotal);
}

int NativeReplayScheduler_RunSelfTest(void)
{
	struct NativeReplayFileHeader executableHeader;
	struct NativeReplayFrameRecord expectedRecord;
	struct NativeReplaySchedulerFrameInfo live;
	const u64 savedExecutableFingerprint = s_executableFingerprint;
	const s32 savedExecutableFingerprintReady = s_executableFingerprintReady;
	long checkpointFrameOffset;
	u32 difference;
	u32 parsedFrame;
	int emittedVBlanks;

	if (!NativeReplayScheduler_ParseU32("17", &parsedFrame) || (parsedFrame != 17u) ||
	    NativeReplayScheduler_ParseU32("17x", &parsedFrame) || NativeReplayScheduler_ParseU32("-1", &parsedFrame))
	{
		fprintf(stderr, "[CTR Replay] self-test failed: replay frame parser\n");
		return 1;
	}
	if (!NativeReplayScheduler_FrameFileOffset(17u, &checkpointFrameOffset) ||
	    (checkpointFrameOffset != (long)sizeof(struct NativeReplayFileHeader) + (17L * (long)sizeof(struct NativeReplayFrameRecord))))
	{
		fprintf(stderr, "[CTR Replay] self-test failed: checkpoint-start frame offset\n");
		return 1;
	}

	NativeReplayScheduler_ResetSessionState();
	memset(&s_inputRecord, 0, sizeof(s_inputRecord));
	memset(&s_pendingRecord, 0, sizeof(s_pendingRecord));
	s_mode = NATIVE_REPLAY_MODE_RECORD;
	s_inputHeader.version = NATIVE_REPLAY_FILE_VERSION;
	s_inputRecord.vblankPackets[0] = (u16)(2u | (1u << NATIVE_REPLAY_VSYNC_PACKET_REPEAT_SHIFT));
	s_inputRecord.vblankPackets[1] = 3;
	s_inputRecord.vblankTotal = 7;
	s_inputRecord.vblankPacketCount = NativeReplayScheduler_EncodeVSyncPacketCounts(1, 2);
	s_inputCompleteTiming = 1;
	s_inputRecordReady = 1;
	if (!NativeReplayScheduler_RecordVSyncLayoutValid(s_inputHeader.version, &s_inputRecord) ||
	    !NativeReplayScheduler_ConsumeVSyncPacket(1, &emittedVBlanks) || (emittedVBlanks != 2) ||
	    (s_preFrameVBlankPacketCount != 1) || (s_inputVBlankPacketCursor != 0) ||
	    (s_inputVBlankPacketRepeatCursor != 1) || !NativeReplayScheduler_ConsumeVSyncPacket(1, &emittedVBlanks) ||
	    (emittedVBlanks != 2) || (s_preFrameVBlankPacketCount != 1) || (s_inputVBlankPacketCursor != 1) ||
	    (s_inputVBlankPacketRepeatCursor != 0))
	{
		fprintf(stderr, "[CTR Replay] self-test failed: complete pre-frame VSync seed\n");
		return 1;
	}
	NativeReplayScheduler_BeginRecordedFrameVSync();
	s_beginOpen = 1;
	if (!NativeReplayScheduler_ConsumeVSyncPacket(1, &emittedVBlanks) || (emittedVBlanks != 3) ||
	    (s_framePreVBlankPacketCount != 1) || (s_frameVBlankPacketCount != 2) || (s_frameVBlankTotal != 7) ||
	    (s_inputVBlankPacketCursor != 2))
	{
		fprintf(stderr, "[CTR Replay] self-test failed: complete in-frame VSync seed\n");
		return 1;
	}
	s_pendingRecord.vblankTotal = s_frameVBlankTotal;
	s_pendingRecord.vblankPacketCount =
	    NativeReplayScheduler_EncodeVSyncPacketCounts(s_framePreVBlankPacketCount, s_frameVBlankPacketCount);
	if (!NativeReplayScheduler_RecordVSyncLayoutValid(NATIVE_REPLAY_FILE_VERSION, &s_pendingRecord) ||
	    (NativeReplayScheduler_RecordPreFrameVSyncPacketCount(NATIVE_REPLAY_FILE_VERSION, &s_pendingRecord) != 1) ||
	    (NativeReplayScheduler_RecordVSyncPacketCount(NATIVE_REPLAY_FILE_VERSION, &s_pendingRecord) != 2))
	{
		fprintf(stderr, "[CTR Replay] self-test failed: complete VSync record encoding\n");
		return 1;
	}

	s_executableFingerprint = UINT64_C(0x0123456789abcdef);
	s_executableFingerprintReady = 1;
	NativeReplayScheduler_InitHeader(&executableHeader);
	if (!NativeReplayScheduler_HeaderIdentityValid(&executableHeader))
	{
		fprintf(stderr, "[CTR Replay] self-test failed: identical executable fingerprint did not match\n");
		return 1;
	}
	s_executableFingerprint ^= UINT64_C(0x1);
	if (NativeReplayScheduler_HeaderIdentityValid(&executableHeader))
	{
		fprintf(stderr, "[CTR Replay] self-test failed: different executable fingerprint matched\n");
		return 1;
	}
	s_executableFingerprint = savedExecutableFingerprint;
	s_executableFingerprintReady = savedExecutableFingerprintReady;

	NativeReplayScheduler_ResetSessionState();
	if ((NativeReplayScheduler_RuntimeFailure() != 1) || (s_exitStatus != NATIVE_REPLAY_EXIT_FAILURE))
	{
		fprintf(stderr, "[CTR Replay] self-test failed: runtime failure exit status\n");
		return 1;
	}

	NativeReplayScheduler_ResetSessionState();
	s_mode = NATIVE_REPLAY_MODE_PLAYBACK;
	s_replayFrame = 16;
	s_header.frameCount = 17;
	NativeReplayScheduler_CheckPlaybackComplete();
	if (s_exitStatus != NATIVE_REPLAY_EXIT_FAILURE)
	{
		fprintf(stderr, "[CTR Replay] self-test failed: incomplete playback exit status\n");
		return 1;
	}

	memset(&expectedRecord, 0, sizeof(expectedRecord));
	expectedRecord.replayFrame = 17;
	expectedRecord.endInfo.stateDigest.schemaVersion = NATIVE_STATE_DIGEST_SCHEMA_VERSION;
	expectedRecord.endInfo.stateDigest.componentMask = NATIVE_STATE_DIGEST_COMPONENT_ALL;
	expectedRecord.endInfo.stateDigest.timing = UINT64_C(0x1111111111111111);
	expectedRecord.endInfo.stateDigest.rng = UINT64_C(0x2222222222222222);
	expectedRecord.endInfo.stateDigest.drivers = UINT64_C(0x3333333333333333);
	expectedRecord.endInfo.stateDigest.world = UINT64_C(0x4444444444444444);
	expectedRecord.endInfo.stateDigest.allocation = UINT64_C(0x5555555555555555);
	expectedRecord.endInfo.stateDigest.root = UINT64_C(0x6666666666666666);
	live = expectedRecord.endInfo;
	if (!NativeReplayScheduler_FrameInfoMatches(&expectedRecord.endInfo, &live))
	{
		fprintf(stderr, "[CTR Replay] self-test failed: identical frame state did not match\n");
		return 1;
	}

	live.stateDigest.drivers ^= 1;
	live.stateDigest.root ^= 1;
	difference = NativeStateDigest_DifferenceMask(&expectedRecord.endInfo.stateDigest, &live.stateDigest);
	if (NativeReplayScheduler_FrameInfoMatches(&expectedRecord.endInfo, &live) ||
	    (difference != NATIVE_STATE_DIGEST_COMPONENT_DRIVERS) ||
	    (strcmp(NativeStateDigest_FirstDifferenceName(difference), "drivers") != 0))
	{
		fprintf(stderr, "[CTR Replay] self-test failed: driver mutation was not isolated\n");
		return 1;
	}

	NativeReplayScheduler_ResetSessionState();
	NativeReplayScheduler_ReportDivergence(&expectedRecord, &live, 0);
	if ((s_exitStatus != NATIVE_REPLAY_EXIT_DIVERGENCE) || (s_divergenceLogged == 0))
	{
		fprintf(stderr, "[CTR Replay] self-test failed: divergence exit status\n");
		return 1;
	}

	printf("[CTR Replay] self-test passed: runtime-error=1 divergence=2 mutation-frame=17 component=drivers binary-identity=checked "
	       "checkpoint-start=checked complete-vsync=checked\n");
	return 0;
}

int NativeReplayScheduler_ConfigureFromArgs(int argc, char **argv)
{
	const char *playbackPath = NativeReplayScheduler_ArgValue(argc, argv, "--replay");
	const char *recordFromReplayPath = NativeReplayScheduler_ArgValue(argc, argv, "--record-from-replay");
	const char *testPerturbFrameText = NativeReplayScheduler_ArgValue(argc, argv, "--replay-test-perturb-driver-x");
	const char *startCheckpointText = NativeReplayScheduler_ArgValue(argc, argv, "--replay-start-checkpoint");
	const char *renderTraceFrameText = NativeReplayScheduler_ArgValue(argc, argv, "--render-trace-frame");
	const s32 recordReport = NativeReplayScheduler_ArgPresent(argc, argv, "--record");
	const s32 recordFromReplay = NativeReplayScheduler_ArgPresent(argc, argv, "--record-from-replay");
	const s32 playback = NativeReplayScheduler_ArgPresent(argc, argv, "--replay");
	const s32 bypassHeaderIdentity = NativeReplayScheduler_ArgPresent(argc, argv, "--replay-bypass-header");
	const s32 testPerturb = NativeReplayScheduler_ArgPresent(argc, argv, "--replay-test-perturb-driver-x");
	const s32 startCheckpoint = NativeReplayScheduler_ArgPresent(argc, argv, "--replay-start-checkpoint");
	const s32 renderTrace = NativeReplayScheduler_ArgPresent(argc, argv, "--render-trace-frame");
	s32 toggle = NativeReplayScheduler_ArgPresent(argc, argv, "--toggle");
	s32 detailed = NativeReplayScheduler_ArgPresent(argc, argv, "--detailed");
	u32 testPerturbFrame = 0;
	u32 startCheckpointIndex = 0;
	u32 renderTraceFrame = 0;

	if (((recordReport != 0) || (recordFromReplay != 0) || (playback != 0)) && (s_executableFingerprintReady == 0))
	{
		Platform_Log("[CTR Replay] cannot identify the running executable; replay is disabled\n");
		return 1;
	}

	if (NativeReplayScheduler_ArgMissingValue(argc, argv, "--replay"))
	{
		Platform_Log("[CTR Replay] missing replay command value\n");
		return 1;
	}
	if (NativeReplayScheduler_ArgMissingValue(argc, argv, "--record-from-replay"))
	{
		Platform_Log("[CTR Replay] missing --record-from-replay command value\n");
		return 1;
	}
	if (NativeReplayScheduler_ArgMissingValue(argc, argv, "--replay-test-perturb-driver-x"))
	{
		Platform_Log("[CTR Replay] missing --replay-test-perturb-driver-x frame value\n");
		return 1;
	}
	if (NativeReplayScheduler_ArgMissingValue(argc, argv, "--replay-start-checkpoint"))
	{
		Platform_Log("[CTR Replay] missing --replay-start-checkpoint value\n");
		return 1;
	}
	if (NativeReplayScheduler_ArgMissingValue(argc, argv, "--render-trace-frame"))
	{
		Platform_Log("[CTR Replay] missing --render-trace-frame value\n");
		return 1;
	}

	if (((recordReport != 0) + (recordFromReplay != 0) + (playback != 0)) > 1)
	{
		Platform_Log("[CTR Replay] choose one of --record, --record-from-replay, or --replay\n");
		return 1;
	}
	if (((toggle != 0) || (detailed != 0)) && (recordReport == 0) && (recordFromReplay == 0))
	{
		Platform_Log("[CTR Replay] --toggle and --detailed only apply to recording\n");
		return 1;
	}
	if ((toggle != 0) && (recordFromReplay != 0))
	{
		Platform_Log("[CTR Replay] --toggle does not apply to --record-from-replay\n");
		return 1;
	}
	if ((bypassHeaderIdentity != 0) && (playback == 0))
	{
		Platform_Log("[CTR Replay] --replay-bypass-header only applies to --replay\n");
		return 1;
	}
	if ((testPerturb != 0) && (playback == 0))
	{
		Platform_Log("[CTR Replay] --replay-test-perturb-driver-x only applies to --replay\n");
		return 1;
	}
	if ((startCheckpoint != 0) && (playback == 0))
	{
		Platform_Log("[CTR Replay] --replay-start-checkpoint only applies to --replay\n");
		return 1;
	}
	if ((renderTrace != 0) && (playback == 0))
	{
		Platform_Log("[CTR Replay] --render-trace-frame only applies to --replay\n");
		return 1;
	}
	if ((testPerturb != 0) && !NativeReplayScheduler_ParseU32(testPerturbFrameText, &testPerturbFrame))
	{
		Platform_Log("[CTR Replay] invalid --replay-test-perturb-driver-x frame: %s\n",
		             testPerturbFrameText != NULL ? testPerturbFrameText : "");
		return 1;
	}
	if ((startCheckpoint != 0) && !NativeReplayScheduler_ParseU32(startCheckpointText, &startCheckpointIndex))
	{
		Platform_Log("[CTR Replay] invalid --replay-start-checkpoint value: %s\n", startCheckpointText != NULL ? startCheckpointText : "");
		return 1;
	}
	if ((renderTrace != 0) && !NativeReplayScheduler_ParseU32(renderTraceFrameText, &renderTraceFrame))
	{
		Platform_Log("[CTR Replay] invalid --render-trace-frame value: %s\n", renderTraceFrameText != NULL ? renderTraceFrameText : "");
		return 1;
	}

	if (((recordReport != 0) || (recordFromReplay != 0)) && (s_reportEnabled == 0) &&
	    !NativeReplayScheduler_PrepareReportPaths(NATIVE_REPLAY_DEFAULT_REPORT_ROOT))
	{
		Platform_Log("[CTR Replay] failed to prepare report folder under: %s\n", NATIVE_REPLAY_DEFAULT_REPORT_ROOT);
		return 1;
	}

	NativeReplayScheduler_ResetSessionState();
	s_startRequested = 0;
	s_reportCompleted = 0;
	s_reportManualStart = 0;
	s_testPerturbEnabled = testPerturb;
	s_testPerturbFrame = testPerturbFrame;
	s_renderTraceEnabled = renderTrace;
	s_renderTraceFrame = renderTraceFrame;
	s_checkpointPolicy = (recordReport != 0) ? NATIVE_REPLAY_CHECKPOINT_POLICY_BOOTSTRAP_ONLY : NATIVE_REPLAY_CHECKPOINT_POLICY_ROLLING;
	if ((detailed != 0) || (recordFromReplay != 0))
	{
		s_checkpointPolicy = NATIVE_REPLAY_CHECKPOINT_POLICY_ROLLING;
	}

	if (recordReport != 0)
	{
		if (toggle == 0)
		{
			return NativeReplayScheduler_OpenRecord(s_reportReplayPath, s_reportCheckpointPath, NULL) ? 0 : 1;
		}

		return NativeReplayScheduler_ArmReport() ? 0 : 1;
	}

	if (recordFromReplayPath != NULL)
	{
		return NativeReplayScheduler_OpenRecordFromReplay(recordFromReplayPath) ? 0 : 1;
	}

	if (playbackPath != NULL)
	{
		return NativeReplayScheduler_OpenPlayback(playbackPath, bypassHeaderIdentity, startCheckpointIndex) ? 0 : 1;
	}

	return 0;
}

void NativeReplayScheduler_Shutdown(void)
{
	NativeReplayScheduler_CloseFiles();
	NativeReplayScheduler_ResetMemcardSandbox();
	NativeReplayScheduler_FreeReportPaths();
	Platform_InputClearInstalledPadSnapshots();
	s_mode = NATIVE_REPLAY_MODE_NONE;
}

int NativeReplayScheduler_RequestStart(void)
{
	if (s_mode == NATIVE_REPLAY_MODE_RECORD)
	{
		Platform_Log("[CTR Replay] report recording is already active\n");
		return 1;
	}
	if (s_mode != NATIVE_REPLAY_MODE_ARMED)
	{
		return 0;
	}
	if (s_reportCompleted != 0)
	{
		Platform_Log("[CTR Replay] report is already finalized\n");
		return 1;
	}

	if (s_startRequested == 0)
	{
		s_startRequested = 1;
		Platform_Log("[CTR Replay] report start requested; recording begins next frame\n");
	}
	return 1;
}

int NativeReplayScheduler_RequestStop(void)
{
	if (s_mode == NATIVE_REPLAY_MODE_ARMED)
	{
		Platform_Log("[CTR Replay] report is armed but not recording; press F9 to start\n");
		return 1;
	}
	if (s_mode != NATIVE_REPLAY_MODE_RECORD)
	{
		return 0;
	}

	if (s_stopRequested == 0)
	{
		s_stopRequested = 1;
		Platform_Log("[CTR Replay] report stop requested; finalizing after current frame\n");
	}
	return 1;
}

int NativeReplayScheduler_BeginFrame(const struct NativeReplaySchedulerFrameInfo *info)
{
	if ((s_mode == NATIVE_REPLAY_MODE_NONE) || (info == NULL))
	{
		return 0;
	}

	if (s_mode == NATIVE_REPLAY_MODE_ARMED)
	{
		if (s_startRequested == 0)
		{
			return 0;
		}

		if (!NativeReplayScheduler_MemcardIdleForRootSwitch())
		{
			NativeReplayScheduler_LogMemcardStartDeferred();
			return 0;
		}

		s_startRequested = 0;
		s_recordStartDeferredLogged = 0;
		if (!NativeReplayScheduler_StartReportRecording())
		{
			return NativeReplayScheduler_RuntimeFailure();
		}
	}

	if (s_mode == NATIVE_REPLAY_MODE_RECORD)
	{
		if ((s_inputFile != NULL) && (s_replayFrame >= s_inputHeader.frameCount))
		{
			Platform_Log("[CTR Replay] replay-seeded recording finished after %u frames\n", s_replayFrame);
			NativeReplayScheduler_CloseFiles();
			return 1;
		}
		if (s_inputFile != NULL)
		{
			if (!NativeReplayScheduler_ReadInputRecord(s_replayFrame))
			{
				return NativeReplayScheduler_RuntimeFailure();
			}
			if ((s_inputCompleteTiming != 0) &&
			    ((s_inputVBlankPlaybackMismatch != 0) ||
			     (s_inputVBlankPacketRepeatCursor != 0) ||
			     (s_inputVBlankPacketCursor !=
			      NativeReplayScheduler_RecordPreFrameVSyncPacketCount(s_inputHeader.version, &s_inputRecord))))
			{
				Platform_Log("[CTR Replay] replay seed pre-frame VSync mismatch at frame %u: expected=%u consumed=%u\n", s_replayFrame,
				             NativeReplayScheduler_RecordPreFrameVSyncPacketCount(s_inputHeader.version, &s_inputRecord),
				             s_inputVBlankPacketCursor);
				return NativeReplayScheduler_RuntimeFailure();
			}
			if (!Platform_InputUpgradeLegacySubmitNameSnapshots(s_inputRecord.pads, PLATFORM_INPUT_PAD_COUNT) ||
			    !Platform_InputInstallPadSnapshots(s_inputRecord.pads, PLATFORM_INPUT_PAD_COUNT))
			{
				Platform_Log("[CTR Replay] failed to install replay seed input frame %u\n", s_replayFrame);
				return NativeReplayScheduler_RuntimeFailure();
			}
		}

		if (!NativeReplayScheduler_WriteCheckpointIfDue())
		{
			return NativeReplayScheduler_RuntimeFailure();
		}

		memset(&s_pendingRecord, 0, sizeof(s_pendingRecord));
		s_pendingRecord.magic = NATIVE_REPLAY_FRAME_MAGIC;
		s_pendingRecord.replayFrame = s_replayFrame;
		s_pendingRecord.beginInfo = *info;
		s_frameTimingConsumed = 0;
		NativeReplayScheduler_BeginRecordedFrameVSync();
		if (Platform_InputCapturePadSnapshots(s_pendingRecord.pads, PLATFORM_INPUT_PAD_COUNT) == 0)
		{
			Platform_Log("[CTR Replay] failed to capture input snapshots\n");
			return NativeReplayScheduler_RuntimeFailure();
		}
		s_pendingRecord.padChecksum = NativeReplayScheduler_PadChecksum(s_pendingRecord.pads);
		s_beginOpen = 1;
		return 0;
	}

	if (s_mode == NATIVE_REPLAY_MODE_PLAYBACK)
	{
		u32 checksum;

		if (!NativeReplayScheduler_RestoreBootstrapCheckpoint())
		{
			return NativeReplayScheduler_RuntimeFailure();
		}

		if (s_replayFrame >= s_header.frameCount)
		{
			if ((s_testPerturbEnabled != 0) && (s_testPerturbApplied == 0))
			{
				Platform_Log("[CTR Replay] test perturbation frame %u was not reached before replay end at frame %u\n", s_testPerturbFrame,
				             s_replayFrame);
				return NativeReplayScheduler_RuntimeFailure();
			}
			Platform_Log("[CTR Replay] replay finished after %u frames\n", s_replayFrame);
			return 1;
		}

		if (fread(&s_pendingRecord, sizeof(s_pendingRecord), 1, s_file) != 1)
		{
			Platform_Log("[CTR Replay] failed to read replay frame %u\n", s_replayFrame);
			return NativeReplayScheduler_RuntimeFailure();
		}

		checksum = NativeReplayScheduler_RecordChecksum(&s_pendingRecord);
		if ((s_pendingRecord.magic != NATIVE_REPLAY_FRAME_MAGIC) || (s_pendingRecord.replayFrame != s_replayFrame) ||
		    (checksum != s_pendingRecord.recordChecksum) ||
		    (NativeReplayScheduler_PadChecksum(s_pendingRecord.pads) != s_pendingRecord.padChecksum) ||
		    !NativeReplayScheduler_RecordVSyncLayoutValid(s_header.version, &s_pendingRecord))
		{
			Platform_Log("[CTR Replay] corrupt replay frame %u\n", s_replayFrame);
			return NativeReplayScheduler_RuntimeFailure();
		}

		if (Platform_InputInstallPadSnapshots(s_pendingRecord.pads, PLATFORM_INPUT_PAD_COUNT) == 0)
		{
			Platform_Log("[CTR Replay] failed to install replay input frame %u\n", s_replayFrame);
			return NativeReplayScheduler_RuntimeFailure();
		}

		s_frameTimingConsumed = 0;
		NativeReplayScheduler_BeginPlaybackFrameVSync(s_header.version, &s_pendingRecord);
		s_beginOpen = 1;
		if ((s_renderTraceEnabled != 0) && (s_replayFrame == s_renderTraceFrame))
		{
			NativeGpu_RenderTraceBegin(s_replayFrame);
		}
	}

	return 0;
}

int NativeReplayScheduler_ConsumeVSyncPacket(int requestedVBlanks, int *emittedVBlanks)
{
	u32 packet;
	u32 packetLimit;

	if (emittedVBlanks == NULL)
	{
		return 0;
	}

	if (requestedVBlanks < 1)
	{
		requestedVBlanks = 1;
	}

	if ((s_mode == NATIVE_REPLAY_MODE_RECORD) && (s_inputCompleteTiming != 0) && (s_inputRecordReady != 0))
	{
		packetLimit = (s_beginOpen != 0)
		                  ? NativeReplayScheduler_RecordVSyncPacketCount(s_inputHeader.version, &s_inputRecord)
		                  : NativeReplayScheduler_RecordPreFrameVSyncPacketCount(s_inputHeader.version, &s_inputRecord);
		if (s_inputVBlankPacketCursor >= packetLimit)
		{
			s_inputVBlankPlaybackMismatch = 1;
			packet = (u32)requestedVBlanks;
		}
		else
		{
			u16 encodedPacket = s_inputRecord.vblankPackets[s_inputVBlankPacketCursor];
			u32 repeatCount = NativeReplayScheduler_VSyncPacketRepeatCount(s_inputHeader.version, encodedPacket);

			packet = NativeReplayScheduler_VSyncPacketValue(s_inputHeader.version, encodedPacket);
			if (packet == 0)
			{
				s_inputVBlankPlaybackMismatch = 1;
				packet = (u32)requestedVBlanks;
			}
			s_inputVBlankPacketRepeatCursor++;
			if (s_inputVBlankPacketRepeatCursor >= repeatCount)
			{
				s_inputVBlankPacketCursor++;
				s_inputVBlankPacketRepeatCursor = 0;
			}
		}

		NativeReplayScheduler_RecordEmittedVSyncPacket((int)packet);
		*emittedVBlanks = (int)packet;
		return 1;
	}

	if ((s_mode != NATIVE_REPLAY_MODE_PLAYBACK) || (s_beginOpen == 0))
	{
		return 0;
	}

	packetLimit = NativeReplayScheduler_RecordVSyncPacketCount(s_header.version, &s_pendingRecord);
	if (s_frameVBlankPacketCount >= packetLimit)
	{
		s_vblankPlaybackMismatch = 1;
		*emittedVBlanks = requestedVBlanks;
		return 1;
	}

	{
		u16 encodedPacket = s_pendingRecord.vblankPackets[s_frameVBlankPacketCount];
		u32 repeatCount = NativeReplayScheduler_VSyncPacketRepeatCount(s_header.version, encodedPacket);

		packet = NativeReplayScheduler_VSyncPacketValue(s_header.version, encodedPacket);
		s_frameVBlankPacketRepeatCursor++;
		if (s_frameVBlankPacketRepeatCursor >= repeatCount)
		{
			s_frameVBlankPacketCount++;
			s_frameVBlankPacketRepeatCursor = 0;
		}
	}
	if (packet == 0)
	{
		s_vblankPlaybackMismatch = 1;
		packet = (u32)requestedVBlanks;
	}

	s_frameVBlankTotal += packet;
	*emittedVBlanks = (int)packet;
	return 1;
}

int NativeReplayScheduler_ConsumeFrameElapsedTimeMS(int *elapsedTimeMS)
{
	if ((s_beginOpen == 0) || (elapsedTimeMS == NULL) || (s_frameTimingConsumed != 0))
	{
		return 0;
	}

	if (s_mode == NATIVE_REPLAY_MODE_PLAYBACK)
	{
		*elapsedTimeMS = s_pendingRecord.endInfo.elapsedTimeMS;
	}
	else if ((s_mode == NATIVE_REPLAY_MODE_RECORD) && (s_inputCompleteTiming != 0) && (s_inputRecordReady != 0))
	{
		*elapsedTimeMS = s_inputRecord.endInfo.elapsedTimeMS;
	}
	else
	{
		return 0;
	}

	s_frameTimingConsumed = 1;
	return 1;
}

void NativeReplayScheduler_RecordVSyncPacket(int emittedVBlanks)
{
	if ((s_mode != NATIVE_REPLAY_MODE_RECORD) || (emittedVBlanks <= 0))
	{
		return;
	}

	NativeReplayScheduler_RecordEmittedVSyncPacket(emittedVBlanks);
}

void NativeReplayScheduler_ObserveGameplayState(const struct GameTracker *gGT)
{
	const s32 active = (gGT != NULL) && (gGT->drivers[0] != NULL);
	const s32 raceActive =
	    active && ((gGT->gameMode1 & (ARCADE_MODE | TIME_TRIAL | ADVENTURE_MODE)) != 0) &&
	    ((gGT->gameMode1 & (MAIN_MENU | GAME_CUTSCENE | LOADING | ADVENTURE_ARENA | END_OF_RACE)) == 0);

	if ((s_mode != NATIVE_REPLAY_MODE_RECORD) && (s_mode != NATIVE_REPLAY_MODE_PLAYBACK))
	{
		return;
	}

	if (active != s_driver0ActiveState)
	{
		Platform_Log("[CTR Replay] driver[0] became %s at replay frame %u\n", active != 0 ? "active" : "inactive", s_replayFrame);
		s_driver0ActiveState = active;
	}

	if (raceActive != s_raceDriver0ActiveState)
	{
		Platform_Log("[CTR Replay] race driver[0] became %s at replay frame %u\n", raceActive != 0 ? "active" : "inactive", s_replayFrame);
		s_raceDriver0ActiveState = raceActive;
	}

}

int NativeReplayScheduler_TestPerturbGameplayState(struct GameTracker *gGT)
{
	struct Driver *driver;
	s32 oldX;

	if ((s_mode != NATIVE_REPLAY_MODE_PLAYBACK) || (s_testPerturbEnabled == 0) || (s_testPerturbApplied != 0) ||
	    (s_replayFrame != s_testPerturbFrame))
	{
		return 0;
	}

	driver = (gGT != NULL) ? gGT->drivers[0] : NULL;
	if (driver == NULL)
	{
		Platform_Log("[CTR Replay] cannot perturb driver position at replay frame %u: driver slot 0 is empty\n", s_replayFrame);
		return NativeReplayScheduler_RuntimeFailure();
	}

	oldX = driver->posCurr.x;
	driver->posCurr.x ^= 1;
	s_testPerturbApplied = 1;
	Platform_Log("[CTR Replay] test perturbation at replay frame %u: driver[0].posCurr.x %d -> %d\n", s_replayFrame, oldX,
	             driver->posCurr.x);
	return 0;
}

int NativeReplayScheduler_EndFrame(const struct NativeReplaySchedulerFrameInfo *info)
{
	struct PlatformInputPadSnapshot livePads[PLATFORM_INPUT_PAD_COUNT];
	u32 livePadChecksum;

	if ((s_mode == NATIVE_REPLAY_MODE_NONE) || (info == NULL))
	{
		return 0;
	}

	if (s_beginOpen == 0)
	{
		return 0;
	}

	if (Platform_InputCapturePadSnapshots(livePads, PLATFORM_INPUT_PAD_COUNT) == 0)
	{
		Platform_Log("[CTR Replay] failed to capture live input snapshots at replay frame %u\n", s_replayFrame);
		return NativeReplayScheduler_RuntimeFailure();
	}
	livePadChecksum = NativeReplayScheduler_PadChecksum(livePads);

	if (s_mode == NATIVE_REPLAY_MODE_RECORD)
	{
		if ((s_vblankPacketOverflow != 0) || (s_preFrameVBlankPacketOverflow != 0))
		{
			Platform_Log("[CTR Replay] too many VSync packets in replay frame %u\n", s_replayFrame);
			return NativeReplayScheduler_RuntimeFailure();
		}
		if ((s_inputCompleteTiming != 0) &&
		    ((s_inputVBlankPlaybackMismatch != 0) ||
		     (s_inputVBlankPacketRepeatCursor != 0) ||
		     (s_inputVBlankPacketCursor != NativeReplayScheduler_RecordVSyncPacketCount(s_inputHeader.version, &s_inputRecord))))
		{
			Platform_Log("[CTR Replay] replay seed in-frame VSync mismatch at frame %u: expected=%u consumed=%u\n", s_replayFrame,
			             NativeReplayScheduler_RecordVSyncPacketCount(s_inputHeader.version, &s_inputRecord),
			             s_inputVBlankPacketCursor);
			return NativeReplayScheduler_RuntimeFailure();
		}

		s_pendingRecord.vblankTotal = s_frameVBlankTotal;
		s_pendingRecord.vblankPacketCount =
		    NativeReplayScheduler_EncodeVSyncPacketCounts(s_framePreVBlankPacketCount, s_frameVBlankPacketCount);
		s_pendingRecord.endInfo = *info;
		s_pendingRecord.recordChecksum = NativeReplayScheduler_RecordChecksum(&s_pendingRecord);

		if (fwrite(&s_pendingRecord, sizeof(s_pendingRecord), 1, s_file) != 1)
		{
			Platform_Log("[CTR Replay] failed to write replay frame %u\n", s_replayFrame);
			return NativeReplayScheduler_RuntimeFailure();
		}

		s_header.frameCount++;
		if (!NativeReplayScheduler_WriteHeader())
		{
			Platform_Log("[CTR Replay] failed to update replay header frame count\n");
			return NativeReplayScheduler_RuntimeFailure();
		}
		s_replayFrame++;
		if (s_inputFile != NULL)
		{
			s_inputRecordReady = 0;
			s_inputVBlankPacketCursor = 0;
			s_inputVBlankPacketRepeatCursor = 0;
			s_inputVBlankPlaybackMismatch = 0;
			if ((s_inputCompleteTiming != 0) && (s_replayFrame < s_inputHeader.frameCount) &&
			    !NativeReplayScheduler_ReadInputRecord(s_replayFrame))
			{
				return NativeReplayScheduler_RuntimeFailure();
			}
		}
		s_beginOpen = 0;
		s_frameTimingConsumed = 0;
		NativeReplayScheduler_ResetVSyncPackets();

		if (s_stopRequested != 0)
		{
			Platform_Log("[CTR Replay] report finalized by hotkey: frames=%u\n", s_replayFrame);
			NativeReplayScheduler_CloseFiles();
			return 0;
		}

		return 0;
	}

	if (s_mode == NATIVE_REPLAY_MODE_PLAYBACK)
	{
		if ((s_renderTraceEnabled != 0) && (s_replayFrame == s_renderTraceFrame))
		{
			NativeGpu_RenderTraceEnd(s_replayFrame);
		}
		if (!NativeReplayScheduler_FrameInfoMatches(&s_pendingRecord.endInfo, info) || !NativeReplayScheduler_VSyncInfoMatches(&s_pendingRecord) ||
		    (s_pendingRecord.padChecksum != livePadChecksum))
		{
			NativeReplayScheduler_ReportDivergence(&s_pendingRecord, info, livePadChecksum);
			return 1;
		}

		s_replayFrame++;
		s_beginOpen = 0;
		s_frameTimingConsumed = 0;
		NativeReplayScheduler_ResetVSyncPackets();
	}

	return 0;
}

int NativeReplayScheduler_GetExitStatus(void)
{
	return s_exitStatus;
}
#endif

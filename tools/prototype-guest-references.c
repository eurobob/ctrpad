#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FOURCC(a, b, c, d) \
	((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#define CHECKPOINT_FILE_MAGIC FOURCC('C', 'T', 'S', 'T')
#define CHECKPOINT_FILE_VERSION 1u
#define CHECKPOINT_MAGIC FOURCC('C', 'T', 'R', 'C')
#define CHECKPOINT_VERSION 2u

#define REGION_SDATA FOURCC('S', 'D', 'A', 'T')
#define REGION_MEMPACK FOURCC('M', 'P', 'A', 'K')
#define REGION_POINTER_MAP FOURCC('P', 'M', 'A', 'P')

#define FILE_HEADER_SIZE 32u
#define FILE_RECORD_HEADER_SIZE 32u
#define CHECKPOINT_HEADER_SIZE 412u
#define CHECKPOINT_RANGE_OFFSET 52u
#define CHECKPOINT_RANGE_SIZE 12u
#define CHECKPOINT_RANGE_CAP 16u
#define CHECKPOINT_REGION_OFFSET 244u
#define CHECKPOINT_REGION_SIZE 12u
#define CHECKPOINT_REGION_CAP 14u
#define POINTER_MAP_HEADER_SIZE 16u
#define POINTER_MAP_RECORD_SIZE 8u

#define SDATA_GAME_TRACKER_OFFSET 0x9bb4u
#define GAME_TRACKER_LEVEL1_OFFSET 0x168u
#define GAME_TRACKER_LEVEL_ID_OFFSET 0x1a18u

#define LEVEL_MESH_INFO_OFFSET 0x00u
#define MESH_NUM_QUADS_OFFSET 0x00u
#define MESH_NUM_VERTICES_OFFSET 0x04u
#define MESH_QUAD_ARRAY_OFFSET 0x0cu
#define MESH_BSP_ROOT_OFFSET 0x18u
#define MESH_NUM_BSP_NODES_OFFSET 0x1cu
#define QUAD_BLOCK_SIZE 0x5cu
#define QUAD_BLOCK_LOW_TEXTURE_OFFSET 0x40u

#define TAGGED_GUEST_REGION_SHIFT 24u
#define TAGGED_GUEST_OFFSET_MASK 0x00ffffffu
#define TAGGED_GUEST_MEMPACK_REGION 1u
#define BOUNDED_GUEST_ARENA_BASE 0x80000000u
#define DIAGNOSTIC_SIZE 256u

struct ByteView
{
	const uint8_t *bytes;
	size_t size;
};

struct CheckpointRecord
{
	uint32_t checkpointIndex;
	uint32_t replayFrame;
	uint32_t payloadOffset;
	uint32_t payloadSize;
	uint32_t checksum;
};

struct CheckpointRange
{
	uint32_t kind;
	uint32_t start;
	uint32_t size;
};

struct CheckpointRegion
{
	uint32_t kind;
	uint32_t offset;
	uint32_t size;
};

struct CheckpointView
{
	struct ByteView payload;
	struct CheckpointRange ranges[CHECKPOINT_RANGE_CAP];
	struct CheckpointRegion regions[CHECKPOINT_REGION_CAP];
	uint32_t rangeCount;
	uint32_t regionCount;
};

static int Fail(const char *format, ...)
{
	va_list args;

	fprintf(stderr, "[CTR GuestRef] error: ");
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputc('\n', stderr);
	return 1;
}

static void SetDiagnostic(char *dst, size_t dstSize, const char *format, ...)
{
	va_list args;

	if ((dst == NULL) || (dstSize == 0))
	{
		return;
	}

	va_start(args, format);
	vsnprintf(dst, dstSize, format, args);
	va_end(args);
}

static int AddSize(size_t left, size_t right, size_t *result)
{
	if ((result == NULL) || (right > SIZE_MAX - left))
	{
		return 0;
	}

	*result = left + right;
	return 1;
}

static int ViewContains(const struct ByteView *view, size_t offset, size_t size)
{
	size_t end;

	return (view != NULL) && AddSize(offset, size, &end) && (end <= view->size);
}

static int ReadU32(const struct ByteView *view, size_t offset, uint32_t *value)
{
	const uint8_t *src;

	if ((value == NULL) || !ViewContains(view, offset, sizeof(*value)))
	{
		return 0;
	}

	src = &view->bytes[offset];
	*value = (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
	return 1;
}

static uint32_t ReadU32Unchecked(const uint8_t *src)
{
	return (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

static uint32_t Fnv1a(const uint8_t *bytes, size_t size)
{
	uint32_t hash = 2166136261u;

	for (size_t i = 0; i < size; i++)
	{
		hash ^= bytes[i];
		hash *= 16777619u;
	}

	return hash;
}

static int ParseU32(const char *text, uint32_t *value)
{
	char *end = NULL;
	unsigned long parsed;

	if ((text == NULL) || (value == NULL) || (text[0] == '\0'))
	{
		return 0;
	}

	errno = 0;
	parsed = strtoul(text, &end, 10);
	if ((errno != 0) || (end == text) || (*end != '\0') || (parsed > UINT32_MAX))
	{
		return 0;
	}

	*value = (uint32_t)parsed;
	return 1;
}

static int ReadFile(const char *path, uint8_t **bytesOut, size_t *sizeOut)
{
	FILE *file;
	long fileSize;
	uint8_t *bytes;

	if ((path == NULL) || (bytesOut == NULL) || (sizeOut == NULL))
	{
		return 0;
	}

	file = fopen(path, "rb");
	if (file == NULL)
	{
		return 0;
	}
	if ((fseek(file, 0, SEEK_END) != 0) || ((fileSize = ftell(file)) < 0) || (fseek(file, 0, SEEK_SET) != 0))
	{
		fclose(file);
		return 0;
	}

	bytes = malloc((size_t)fileSize);
	if ((bytes == NULL) || (fread(bytes, 1, (size_t)fileSize, file) != (size_t)fileSize))
	{
		free(bytes);
		fclose(file);
		return 0;
	}
	if (fclose(file) != 0)
	{
		free(bytes);
		return 0;
	}

	*bytesOut = bytes;
	*sizeOut = (size_t)fileSize;
	return 1;
}

static int FindRecord(const struct ByteView *file, uint32_t requestedFrame, struct CheckpointRecord *recordOut)
{
	uint32_t magic;
	uint32_t version;
	uint32_t headerSize;
	uint32_t recordHeaderSize;
	uint32_t recordCount;
	size_t recordOffset;

	if (!ReadU32(file, 0, &magic) || !ReadU32(file, 4, &version) || !ReadU32(file, 8, &headerSize) ||
	    !ReadU32(file, 12, &recordHeaderSize) || !ReadU32(file, 16, &recordCount))
	{
		return 0;
	}
	if ((magic != CHECKPOINT_FILE_MAGIC) || (version != CHECKPOINT_FILE_VERSION) || (headerSize != FILE_HEADER_SIZE) ||
	    (recordHeaderSize != FILE_RECORD_HEADER_SIZE))
	{
		return 0;
	}

	recordOffset = headerSize;
	for (uint32_t i = 0; i < recordCount; i++)
	{
		struct CheckpointRecord record;
		size_t payloadEnd;

		if (!ReadU32(file, recordOffset + 0, &record.checkpointIndex) || !ReadU32(file, recordOffset + 4, &record.replayFrame) ||
		    !ReadU32(file, recordOffset + 8, &record.payloadOffset) || !ReadU32(file, recordOffset + 12, &record.payloadSize) ||
		    !ReadU32(file, recordOffset + 16, &record.checksum))
		{
			return 0;
		}
		if ((record.payloadOffset != recordOffset + recordHeaderSize) ||
		    !AddSize(record.payloadOffset, record.payloadSize, &payloadEnd) || (payloadEnd > file->size))
		{
			return 0;
		}

		if (record.replayFrame == requestedFrame)
		{
			*recordOut = record;
			return 1;
		}
		recordOffset = payloadEnd;
	}

	return 0;
}

static int ParseCheckpoint(const struct ByteView *payload, struct CheckpointView *checkpoint)
{
	uint32_t magic;
	uint32_t version;
	uint32_t payloadSize;

	if ((payload == NULL) || (checkpoint == NULL) || !ReadU32(payload, 0, &magic) || !ReadU32(payload, 4, &version) ||
	    !ReadU32(payload, 8, &payloadSize) || !ReadU32(payload, 12, &checkpoint->regionCount) ||
	    !ReadU32(payload, 44, &checkpoint->rangeCount))
	{
		return 0;
	}
	if ((magic != CHECKPOINT_MAGIC) || (version != CHECKPOINT_VERSION) || (payloadSize != payload->size) ||
	    (payloadSize < CHECKPOINT_HEADER_SIZE) || (checkpoint->rangeCount > CHECKPOINT_RANGE_CAP) ||
	    (checkpoint->regionCount > CHECKPOINT_REGION_CAP))
	{
		return 0;
	}

	checkpoint->payload = *payload;
	for (uint32_t i = 0; i < checkpoint->rangeCount; i++)
	{
		const size_t offset = CHECKPOINT_RANGE_OFFSET + (size_t)i * CHECKPOINT_RANGE_SIZE;
		struct CheckpointRange *range = &checkpoint->ranges[i];

		if (!ReadU32(payload, offset + 0, &range->kind) || !ReadU32(payload, offset + 4, &range->start) ||
		    !ReadU32(payload, offset + 8, &range->size) || (range->size == 0))
		{
			return 0;
		}
	}
	for (uint32_t i = 0; i < checkpoint->regionCount; i++)
	{
		const size_t offset = CHECKPOINT_REGION_OFFSET + (size_t)i * CHECKPOINT_REGION_SIZE;
		struct CheckpointRegion *region = &checkpoint->regions[i];

		if (!ReadU32(payload, offset + 0, &region->kind) || !ReadU32(payload, offset + 4, &region->offset) ||
		    !ReadU32(payload, offset + 8, &region->size) || !ViewContains(payload, region->offset, region->size))
		{
			return 0;
		}
	}

	return 1;
}

static const struct CheckpointRange *FindRange(const struct CheckpointView *checkpoint, uint32_t kind)
{
	for (uint32_t i = 0; i < checkpoint->rangeCount; i++)
	{
		if (checkpoint->ranges[i].kind == kind)
		{
			return &checkpoint->ranges[i];
		}
	}

	return NULL;
}

static const struct CheckpointRegion *FindRegion(const struct CheckpointView *checkpoint, uint32_t kind)
{
	for (uint32_t i = 0; i < checkpoint->regionCount; i++)
	{
		if (checkpoint->regions[i].kind == kind)
		{
			return &checkpoint->regions[i];
		}
	}

	return NULL;
}

static int CapturedPointerToOffset(const struct CheckpointRange *range, uint32_t capturedPointer, size_t accessSize,
				   uint32_t *offsetOut, char *diagnostic, size_t diagnosticSize, const char *context)
{
	const uint64_t rangeEnd = (uint64_t)range->start + range->size;
	const uint64_t accessEnd = (uint64_t)capturedPointer + accessSize;

	if ((capturedPointer < range->start) || (accessEnd > rangeEnd))
	{
		SetDiagnostic(diagnostic, diagnosticSize,
			      "%s: captured pointer 0x%08" PRIx32 " + %zu is outside [0x%08" PRIx32 ", 0x%08" PRIx64 ")",
			      context, capturedPointer, accessSize, range->start, rangeEnd);
		return 0;
	}

	*offsetOut = capturedPointer - range->start;
	return 1;
}

static int ResolveOffset(uint8_t *arena, uint32_t arenaSize, uint32_t offset, size_t accessSize, uint8_t **pointerOut,
			 char *diagnostic, size_t diagnosticSize, const char *context)
{
	const uint64_t end = (uint64_t)offset + accessSize;

	if (end > arenaSize)
	{
		SetDiagnostic(diagnostic, diagnosticSize,
			      "%s: guest offset 0x%08" PRIx32 " + %zu exceeds arena size 0x%08" PRIx32,
			      context, offset, accessSize, arenaSize);
		return 0;
	}

	*pointerOut = &arena[offset];
	return 1;
}

static int EncodeTaggedGuestRef(uint32_t regionTag, uint32_t arenaSize, uint32_t offset, uint32_t *referenceOut,
				char *diagnostic, size_t diagnosticSize, const char *context)
{
	if ((regionTag == 0) || (regionTag > UINT8_MAX) || (offset > TAGGED_GUEST_OFFSET_MASK) || (offset >= arenaSize))
	{
		SetDiagnostic(diagnostic, diagnosticSize,
			      "%s: region 0x%02" PRIx32 " offset 0x%08" PRIx32
			      " cannot be encoded as an 8:24 tagged reference for arena size 0x%08" PRIx32,
			      context, regionTag, offset, arenaSize);
		return 0;
	}

	*referenceOut = (regionTag << TAGGED_GUEST_REGION_SHIFT) | offset;
	return 1;
}

static int ResolveTaggedGuestRef(uint8_t *arena, uint32_t arenaSize, uint32_t expectedRegionTag, uint32_t reference,
				 size_t accessSize, uint8_t **pointerOut, char *diagnostic, size_t diagnosticSize,
				 const char *context)
{
	const uint32_t regionTag = reference >> TAGGED_GUEST_REGION_SHIFT;
	const uint32_t offset = reference & TAGGED_GUEST_OFFSET_MASK;

	if (reference == 0)
	{
		SetDiagnostic(diagnostic, diagnosticSize, "%s: null tagged reference is not valid here", context);
		return 0;
	}
	if (regionTag != expectedRegionTag)
	{
		SetDiagnostic(diagnostic, diagnosticSize,
			      "%s: tagged reference 0x%08" PRIx32 " has region 0x%02" PRIx32
			      ", expected 0x%02" PRIx32,
			      context, reference, regionTag, expectedRegionTag);
		return 0;
	}

	return ResolveOffset(arena, arenaSize, offset, accessSize, pointerOut, diagnostic, diagnosticSize, context);
}

static int EncodeBoundedGuestAddress(uint32_t arenaSize, uint32_t offset, uint32_t *addressOut, char *diagnostic,
				     size_t diagnosticSize, const char *context)
{
	const uint64_t address = (uint64_t)BOUNDED_GUEST_ARENA_BASE + offset;

	if ((offset >= arenaSize) || (address > UINT32_MAX))
	{
		SetDiagnostic(diagnostic, diagnosticSize,
			      "%s: offset 0x%08" PRIx32 " cannot be encoded in bounded guest arena [0x%08x, +0x%08" PRIx32 ")",
			      context, offset, BOUNDED_GUEST_ARENA_BASE, arenaSize);
		return 0;
	}

	*addressOut = (uint32_t)address;
	return 1;
}

static int ResolveBoundedGuestAddress(uint8_t *arena, uint32_t arenaSize, uint32_t address, size_t accessSize,
				      uint8_t **pointerOut, char *diagnostic, size_t diagnosticSize, const char *context)
{
	if (address < BOUNDED_GUEST_ARENA_BASE)
	{
		SetDiagnostic(diagnostic, diagnosticSize, "%s: guest address 0x%08" PRIx32 " is below base 0x%08x",
			      context, address, BOUNDED_GUEST_ARENA_BASE);
		return 0;
	}

	return ResolveOffset(arena, arenaSize, address - BOUNDED_GUEST_ARENA_BASE, accessSize, pointerOut,
			     diagnostic, diagnosticSize, context);
}

static int ResolveCapturedWithBothDesigns(const struct CheckpointRange *range, uint8_t *arena, uint32_t arenaSize,
					  uint32_t capturedPointer, size_t accessSize, uint32_t *offsetOut,
					  uint8_t **pointerOut, char *diagnostic, size_t diagnosticSize,
					  const char *context)
{
	uint32_t offset;
	uint32_t taggedReference;
	uint32_t boundedAddress;
	uint8_t *taggedPointer;
	uint8_t *boundedPointer;

	if (!CapturedPointerToOffset(range, capturedPointer, accessSize, &offset, diagnostic, diagnosticSize, context) ||
	    !EncodeTaggedGuestRef(TAGGED_GUEST_MEMPACK_REGION, arenaSize, offset, &taggedReference, diagnostic,
				  diagnosticSize, context) ||
	    !ResolveTaggedGuestRef(arena, arenaSize, TAGGED_GUEST_MEMPACK_REGION, taggedReference, accessSize,
				   &taggedPointer, diagnostic, diagnosticSize, context) ||
	    !EncodeBoundedGuestAddress(arenaSize, offset, &boundedAddress, diagnostic, diagnosticSize, context) ||
	    !ResolveBoundedGuestAddress(arena, arenaSize, boundedAddress, accessSize, &boundedPointer, diagnostic,
					diagnosticSize, context))
	{
		return 0;
	}
	if (taggedPointer != boundedPointer)
	{
		SetDiagnostic(diagnostic, diagnosticSize,
			      "%s: tagged and bounded-arena designs resolved to different host pointers", context);
		return 0;
	}

	*offsetOut = offset;
	*pointerOut = taggedPointer;
	return 1;
}

static int PointerMapContains(const struct CheckpointView *checkpoint, const struct CheckpointRegion *pointerMap,
			      uint32_t slotRegion, uint32_t slotOffset, uint32_t *countOut)
{
	struct ByteView map;
	uint32_t count;

	map.bytes = &checkpoint->payload.bytes[pointerMap->offset];
	map.size = pointerMap->size;
	if ((map.size < POINTER_MAP_HEADER_SIZE) || !ReadU32(&map, 0, &count) ||
	    (count > (map.size - POINTER_MAP_HEADER_SIZE) / POINTER_MAP_RECORD_SIZE))
	{
		return 0;
	}
	if (countOut != NULL)
	{
		*countOut = count;
	}

	for (uint32_t i = 0; i < count; i++)
	{
		uint32_t kind;
		uint32_t offset;
		const size_t recordOffset = POINTER_MAP_HEADER_SIZE + (size_t)i * POINTER_MAP_RECORD_SIZE;

		if (!ReadU32(&map, recordOffset + 0, &kind) || !ReadU32(&map, recordOffset + 4, &offset))
		{
			return 0;
		}
		if ((kind == slotRegion) && (offset == slotOffset))
		{
			return 1;
		}
	}

	return 0;
}

static int RunPrototype(const struct ByteView *file, uint32_t requestedFrame)
{
	struct CheckpointRecord record;
	struct ByteView payload;
	struct CheckpointView checkpoint;
	const struct CheckpointRange *mempackRange;
	const struct CheckpointRegion *mempackRegion;
	const struct CheckpointRegion *sdataRegion;
	const struct CheckpointRegion *pointerMapRegion;
	uint8_t *arena = NULL;
	uint8_t *levelPointer;
	uint8_t *meshPointer;
	uint8_t *quadPointer;
	uint8_t *texturePointer;
	uint8_t *boundedPointer;
	uint32_t levelCaptured;
	uint32_t levelOffset;
	uint32_t meshCaptured;
	uint32_t meshOffset;
	uint32_t quadCaptured;
	uint32_t quadOffset;
	uint32_t textureCaptured = 0;
	uint32_t textureOffset = 0;
	uint32_t selectedQuad = UINT32_MAX;
	uint32_t numQuads;
	uint32_t numVertices;
	uint32_t numBspNodes;
	uint32_t levelId;
	uint32_t pointerMapCount = 0;
	char diagnostic[DIAGNOSTIC_SIZE];
	int result = 1;

	if (!FindRecord(file, requestedFrame, &record))
	{
		return Fail("checkpoint frame %" PRIu32 " was not found or the state container is invalid", requestedFrame);
	}

	payload.bytes = &file->bytes[record.payloadOffset];
	payload.size = record.payloadSize;
	if (Fnv1a(payload.bytes, payload.size) != record.checksum)
	{
		return Fail("checkpoint frame %" PRIu32 " checksum does not match its record", requestedFrame);
	}
	if (!ParseCheckpoint(&payload, &checkpoint))
	{
		return Fail("checkpoint frame %" PRIu32 " has an invalid payload header", requestedFrame);
	}

	mempackRange = FindRange(&checkpoint, REGION_MEMPACK);
	mempackRegion = FindRegion(&checkpoint, REGION_MEMPACK);
	sdataRegion = FindRegion(&checkpoint, REGION_SDATA);
	pointerMapRegion = FindRegion(&checkpoint, REGION_POINTER_MAP);
	if ((mempackRange == NULL) || (mempackRegion == NULL) || (sdataRegion == NULL) || (pointerMapRegion == NULL) ||
	    (mempackRange->size != mempackRegion->size))
	{
		return Fail("checkpoint frame %" PRIu32 " is missing compatible SDAT, MPAK, or PMAP metadata", requestedFrame);
	}
	if (!ViewContains(&checkpoint.payload, sdataRegion->offset + SDATA_GAME_TRACKER_OFFSET + GAME_TRACKER_LEVEL1_OFFSET, 4) ||
	    !ViewContains(&checkpoint.payload, sdataRegion->offset + SDATA_GAME_TRACKER_OFFSET + GAME_TRACKER_LEVEL_ID_OFFSET, 4))
	{
		return Fail("checkpoint frame %" PRIu32 " has an incompatible NTSC-U sData/GameTracker layout", requestedFrame);
	}
	if (sizeof(void *) < 8)
	{
		return Fail("the prototype must run in a 64-bit host process");
	}

	arena = malloc(mempackRegion->size);
	if (arena == NULL)
	{
		return Fail("could not allocate the 64-bit prototype arena");
	}
	memcpy(arena, &checkpoint.payload.bytes[mempackRegion->offset], mempackRegion->size);
	if ((uintptr_t)arena <= UINT32_MAX)
	{
		Fail("prototype arena unexpectedly fits in 32 bits (%p); a high host address is required for this proof", (void *)arena);
		goto cleanup;
	}

	levelCaptured = ReadU32Unchecked(&checkpoint.payload.bytes[
	    sdataRegion->offset + SDATA_GAME_TRACKER_OFFSET + GAME_TRACKER_LEVEL1_OFFSET]);
	levelId = ReadU32Unchecked(&checkpoint.payload.bytes[
	    sdataRegion->offset + SDATA_GAME_TRACKER_OFFSET + GAME_TRACKER_LEVEL_ID_OFFSET]);
	if (!ResolveCapturedWithBothDesigns(mempackRange, arena, mempackRegion->size, levelCaptured, 0x194u,
					    &levelOffset, &levelPointer, diagnostic, sizeof(diagnostic),
					    "GameTracker.level1"))
	{
		Fail("%s", diagnostic);
		goto cleanup;
	}

	meshCaptured = ReadU32Unchecked(&levelPointer[LEVEL_MESH_INFO_OFFSET]);
	if (!ResolveCapturedWithBothDesigns(mempackRange, arena, mempackRegion->size, meshCaptured, 0x20u,
					    &meshOffset, &meshPointer, diagnostic, sizeof(diagnostic),
					    "Level.ptr_mesh_info"))
	{
		Fail("%s", diagnostic);
		goto cleanup;
	}

	numQuads = ReadU32Unchecked(&meshPointer[MESH_NUM_QUADS_OFFSET]);
	numVertices = ReadU32Unchecked(&meshPointer[MESH_NUM_VERTICES_OFFSET]);
	numBspNodes = ReadU32Unchecked(&meshPointer[MESH_NUM_BSP_NODES_OFFSET]);
	quadCaptured = ReadU32Unchecked(&meshPointer[MESH_QUAD_ARRAY_OFFSET]);
	if ((numQuads == 0) ||
	    !ResolveCapturedWithBothDesigns(mempackRange, arena, mempackRegion->size, quadCaptured,
					    (size_t)numQuads * QUAD_BLOCK_SIZE, &quadOffset, &quadPointer,
					    diagnostic, sizeof(diagnostic), "mesh_info.ptrQuadBlockArray"))
	{
		Fail("%s", diagnostic);
		goto cleanup;
	}

	for (uint32_t i = 0; i < numQuads; i++)
	{
		const uint32_t candidateCaptured =
		    ReadU32Unchecked(&quadPointer[(size_t)i * QUAD_BLOCK_SIZE + QUAD_BLOCK_LOW_TEXTURE_OFFSET]);
		uint32_t candidateOffset;

		if ((candidateCaptured != 0) &&
		    CapturedPointerToOffset(mempackRange, candidateCaptured, 4, &candidateOffset, diagnostic,
					    sizeof(diagnostic), "QuadBlock.ptr_texture_low") &&
		    PointerMapContains(&checkpoint, pointerMapRegion, REGION_MEMPACK,
				       quadOffset + i * QUAD_BLOCK_SIZE + QUAD_BLOCK_LOW_TEXTURE_OFFSET, NULL))
		{
			selectedQuad = i;
			textureCaptured = candidateCaptured;
			textureOffset = candidateOffset;
			break;
		}
	}
	if ((selectedQuad == UINT32_MAX) ||
	    !ResolveCapturedWithBothDesigns(mempackRange, arena, mempackRegion->size, textureCaptured, 16,
					    &textureOffset, &texturePointer, diagnostic, sizeof(diagnostic),
					    "QuadBlock.ptr_texture_low"))
	{
		Fail("no registered, in-range low-texture reference was found in %" PRIu32 " real quad blocks", numQuads);
		goto cleanup;
	}

	if (!PointerMapContains(&checkpoint, pointerMapRegion, REGION_MEMPACK, levelOffset + LEVEL_MESH_INFO_OFFSET,
				&pointerMapCount) ||
	    !PointerMapContains(&checkpoint, pointerMapRegion, REGION_MEMPACK, meshOffset + MESH_QUAD_ARRAY_OFFSET,
				NULL))
	{
		Fail("the real Level -> mesh_info -> QuadBlock traversal did not use registered relocation slots");
		goto cleanup;
	}

	diagnostic[0] = '\0';
	if (ResolveOffset(arena, mempackRegion->size, mempackRegion->size, 4, &boundedPointer, diagnostic,
			  sizeof(diagnostic), "corrupt offset probe"))
	{
		Fail("out-of-range offset was unexpectedly accepted");
		goto cleanup;
	}
	printf("[CTR GuestRef] rejected offset: %s\n", diagnostic);

	diagnostic[0] = '\0';
	if (ResolveTaggedGuestRef(arena, mempackRegion->size, TAGGED_GUEST_MEMPACK_REGION,
				  (TAGGED_GUEST_MEMPACK_REGION << TAGGED_GUEST_REGION_SHIFT) | mempackRegion->size,
				  4, &boundedPointer, diagnostic, sizeof(diagnostic), "corrupt tagged-reference probe"))
	{
		Fail("out-of-range tagged guest reference was unexpectedly accepted");
		goto cleanup;
	}
	printf("[CTR GuestRef] rejected tagged reference: %s\n", diagnostic);

	diagnostic[0] = '\0';
	if (ResolveBoundedGuestAddress(arena, mempackRegion->size,
				       BOUNDED_GUEST_ARENA_BASE + mempackRegion->size, 4, &boundedPointer,
				       diagnostic, sizeof(diagnostic), "corrupt bounded-address probe"))
	{
		Fail("out-of-range bounded guest address was unexpectedly accepted");
		goto cleanup;
	}
	printf("[CTR GuestRef] rejected bounded address: %s\n", diagnostic);

	diagnostic[0] = '\0';
	if (CapturedPointerToOffset(mempackRange, mempackRange->start - 1u, 4, &textureOffset, diagnostic,
				    sizeof(diagnostic), "corrupt captured-pointer probe"))
	{
		Fail("out-of-range captured pointer was unexpectedly accepted");
		goto cleanup;
	}
	printf("[CTR GuestRef] rejected captured pointer: %s\n", diagnostic);

	printf("[CTR GuestRef] host arena=%p (>32-bit) captured-base=0x%08" PRIx32
	       " bytes=0x%08" PRIx32 " pointer-slots=%" PRIu32 "\n",
	       (void *)arena, mempackRange->start, mempackRange->size, pointerMapCount);
	printf("[CTR GuestRef] real frame=%" PRIu32 " level=%" PRIu32 " quads=%" PRIu32
	       " vertices=%" PRIu32 " bsp-nodes=%" PRIu32 " selected-quad=%" PRIu32 "\n",
	       requestedFrame, levelId, numQuads, numVertices, numBspNodes, selectedQuad);
	printf("[CTR GuestRef] traversal offsets: level=0x%08" PRIx32 " mesh=0x%08" PRIx32
	       " quads=0x%08" PRIx32 " texture=0x%08" PRIx32 " texture-fnv=0x%08" PRIx32 "\n",
	       levelOffset, meshOffset, quadOffset, textureCaptured - mempackRange->start, Fnv1a(texturePointer, 16));
	printf("[CTR GuestRef] prototype passed: tagged and bounded-arena references traversed real relocated data without a host-pointer narrowing\n");
	result = 0;

cleanup:
	free(arena);
	return result;
}

int main(int argc, char **argv)
{
	uint8_t *bytes = NULL;
	size_t size = 0;
	struct ByteView file;
	uint32_t requestedFrame;
	int result;

	if ((argc != 3) || !ParseU32(argv[2], &requestedFrame))
	{
		fprintf(stderr, "Usage: %s STATE_CTRSTATES REPLAY_FRAME\n", argv[0]);
		return 1;
	}
	if (!ReadFile(argv[1], &bytes, &size))
	{
		return Fail("could not read state container: %s", argv[1]);
	}

	file.bytes = bytes;
	file.size = size;
	result = RunPrototype(&file, requestedFrame);
	free(bytes);
	return result;
}

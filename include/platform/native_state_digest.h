#ifndef PLATFORM_NATIVE_STATE_DIGEST_H
#define PLATFORM_NATIVE_STATE_DIGEST_H

#include <macros.h>

#define NATIVE_STATE_DIGEST_SCHEMA_VERSION 2u

enum NativeStateDigestComponent
{
	NATIVE_STATE_DIGEST_COMPONENT_TIMING = 1u << 0,
	NATIVE_STATE_DIGEST_COMPONENT_RNG = 1u << 1,
	NATIVE_STATE_DIGEST_COMPONENT_DRIVERS = 1u << 2,
	NATIVE_STATE_DIGEST_COMPONENT_WORLD = 1u << 3,
	NATIVE_STATE_DIGEST_COMPONENT_ALLOCATION = 1u << 4,
	NATIVE_STATE_DIGEST_COMPONENT_ALL = (1u << 5) - 1u,
};

#define NATIVE_STATE_DIGEST_DIFFERENCE_SCHEMA UINT32_C(0x80000000)

// Fixed-width, explicitly serialized component hashes. This structure contains
// no native pointers, host handles, padding-dependent payloads, or wall-clock
// values, so it has the same 56-byte representation on i686 and ARM64.
struct NativeStateDigest
{
	u32 schemaVersion;
	u32 componentMask;
	u64 timing;
	u64 rng;
	u64 drivers;
	u64 world;
	u64 allocation;
	u64 root;
};

struct GameTracker;

void NativeStateDigest_Capture(const struct GameTracker *gGT, struct NativeStateDigest *out);
u32 NativeStateDigest_DifferenceMask(const struct NativeStateDigest *expected, const struct NativeStateDigest *live);
const char *NativeStateDigest_FirstDifferenceName(u32 differenceMask);
int NativeStateDigest_RunSelfTest(void);

#endif

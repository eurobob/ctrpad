#include "platform/native_state_digest.h"

#include <common.h>

#include <stdio.h>
#include <string.h>

#define NATIVE_STATE_DIGEST_FOURCC(a, b, c, d) ((u32)(a) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))
#define NATIVE_STATE_DIGEST_FNV_OFFSET          UINT64_C(14695981039346656037)
#define NATIVE_STATE_DIGEST_FNV_PRIME           UINT64_C(1099511628211)

enum NativeStateDigestGuestRegion
{
	NATIVE_STATE_DIGEST_GUEST_NONE = 0,
	NATIVE_STATE_DIGEST_GUEST_EXTERNAL = NATIVE_STATE_DIGEST_FOURCC('E', 'X', 'T', 'N'),
	NATIVE_STATE_DIGEST_GUEST_DRIVER = NATIVE_STATE_DIGEST_FOURCC('D', 'R', 'V', 'R'),
	NATIVE_STATE_DIGEST_GUEST_QUADBLOCK = NATIVE_STATE_DIGEST_FOURCC('Q', 'U', 'A', 'D'),
	NATIVE_STATE_DIGEST_GUEST_MEMPACK = NATIVE_STATE_DIGEST_FOURCC('M', 'P', 'A', 'K'),
};

struct NativeStateDigestHasher
{
	u64 value;
};

CTR_STATIC_ASSERT(sizeof(struct NativeStateDigest) == 56);
CTR_STATIC_ASSERT(OFFSETOF(struct NativeStateDigest, timing) == 8);
CTR_STATIC_ASSERT(OFFSETOF(struct NativeStateDigest, root) == 48);

internal struct NativeStateDigestHasher NativeStateDigest_Begin(u32 componentTag)
{
	struct NativeStateDigestHasher hasher;

	hasher.value = NATIVE_STATE_DIGEST_FNV_OFFSET;
	for (u32 shift = 0; shift < 32; shift += 8)
	{
		hasher.value ^= (u8)(componentTag >> shift);
		hasher.value *= NATIVE_STATE_DIGEST_FNV_PRIME;
	}
	return hasher;
}

internal void NativeStateDigest_PutU8(struct NativeStateDigestHasher *hasher, u8 value)
{
	hasher->value ^= value;
	hasher->value *= NATIVE_STATE_DIGEST_FNV_PRIME;
}

internal void NativeStateDigest_PutU16(struct NativeStateDigestHasher *hasher, u16 value)
{
	NativeStateDigest_PutU8(hasher, (u8)value);
	NativeStateDigest_PutU8(hasher, (u8)(value >> 8));
}

internal void NativeStateDigest_PutU32(struct NativeStateDigestHasher *hasher, u32 value)
{
	NativeStateDigest_PutU16(hasher, (u16)value);
	NativeStateDigest_PutU16(hasher, (u16)(value >> 16));
}

internal void NativeStateDigest_PutU64(struct NativeStateDigestHasher *hasher, u64 value)
{
	NativeStateDigest_PutU32(hasher, (u32)value);
	NativeStateDigest_PutU32(hasher, (u32)(value >> 32));
}

internal void NativeStateDigest_PutS16(struct NativeStateDigestHasher *hasher, s16 value)
{
	NativeStateDigest_PutU16(hasher, (u16)value);
}

internal void NativeStateDigest_PutS32(struct NativeStateDigestHasher *hasher, s32 value)
{
	NativeStateDigest_PutU32(hasher, (u32)value);
}

internal void NativeStateDigest_PutSVec3(struct NativeStateDigestHasher *hasher, const SVec3 *value)
{
	NativeStateDigest_PutS16(hasher, value->x);
	NativeStateDigest_PutS16(hasher, value->y);
	NativeStateDigest_PutS16(hasher, value->z);
}

internal void NativeStateDigest_PutSVec3Slot(struct NativeStateDigestHasher *hasher, const SVec3Slot *value)
{
	NativeStateDigest_PutS16(hasher, value->x);
	NativeStateDigest_PutS16(hasher, value->y);
	NativeStateDigest_PutS16(hasher, value->z);
}

internal void NativeStateDigest_PutVec3(struct NativeStateDigestHasher *hasher, const Vec3 *value)
{
	NativeStateDigest_PutS32(hasher, value->x);
	NativeStateDigest_PutS32(hasher, value->y);
	NativeStateDigest_PutS32(hasher, value->z);
}

internal void NativeStateDigest_PutGuestRef(struct NativeStateDigestHasher *hasher, u32 region, u32 offset)
{
	NativeStateDigest_PutU32(hasher, region);
	NativeStateDigest_PutU32(hasher, offset);
}

internal void NativeStateDigest_PutDriverRef(struct NativeStateDigestHasher *hasher, const struct GameTracker *gGT, const struct Driver *driver)
{
	if (driver == NULL)
	{
		NativeStateDigest_PutGuestRef(hasher, NATIVE_STATE_DIGEST_GUEST_NONE, 0);
		return;
	}

	for (u32 i = 0; i < len(gGT->drivers); i++)
	{
		if (gGT->drivers[i] == driver)
		{
			NativeStateDigest_PutGuestRef(hasher, NATIVE_STATE_DIGEST_GUEST_DRIVER, i);
			return;
		}
	}

	NativeStateDigest_PutGuestRef(hasher, NATIVE_STATE_DIGEST_GUEST_EXTERNAL, 0);
}

internal void NativeStateDigest_PutQuadBlockRef(struct NativeStateDigestHasher *hasher, const struct GameTracker *gGT, const struct QuadBlock *quad)
{
	const struct mesh_info *mesh;
	uintptr_t base;
	uintptr_t address;
	uintptr_t byteSize;
	uintptr_t byteOffset;

	if (quad == NULL)
	{
		NativeStateDigest_PutGuestRef(hasher, NATIVE_STATE_DIGEST_GUEST_NONE, 0);
		return;
	}
	if ((gGT->level1 == NULL) || (gGT->level1->ptr_mesh_info == NULL))
	{
		NativeStateDigest_PutGuestRef(hasher, NATIVE_STATE_DIGEST_GUEST_EXTERNAL, 0);
		return;
	}

	mesh = gGT->level1->ptr_mesh_info;
	if ((mesh->ptrQuadBlockArray == NULL) || (mesh->numQuadBlock <= 0) ||
	    ((u32)mesh->numQuadBlock > UINT32_MAX / (u32)sizeof(struct QuadBlock)))
	{
		NativeStateDigest_PutGuestRef(hasher, NATIVE_STATE_DIGEST_GUEST_EXTERNAL, 0);
		return;
	}

	base = (uintptr_t)mesh->ptrQuadBlockArray;
	address = (uintptr_t)quad;
	byteSize = (uintptr_t)(u32)mesh->numQuadBlock * sizeof(struct QuadBlock);
	if ((address < base) || ((address - base) >= byteSize))
	{
		NativeStateDigest_PutGuestRef(hasher, NATIVE_STATE_DIGEST_GUEST_EXTERNAL, 0);
		return;
	}

	byteOffset = address - base;
	if ((byteOffset % sizeof(struct QuadBlock)) != 0)
	{
		NativeStateDigest_PutGuestRef(hasher, NATIVE_STATE_DIGEST_GUEST_EXTERNAL, 0);
		return;
	}

	NativeStateDigest_PutGuestRef(hasher, NATIVE_STATE_DIGEST_GUEST_QUADBLOCK, (u32)byteOffset);
}

internal void NativeStateDigest_PutMempackRef(struct NativeStateDigestHasher *hasher, const void *ptr)
{
	const struct PlatformMempackArena *arena = Platform_GetMempackArena();
	uintptr_t base;
	uintptr_t address;

	if (ptr == NULL)
	{
		NativeStateDigest_PutGuestRef(hasher, NATIVE_STATE_DIGEST_GUEST_NONE, 0);
		return;
	}
	if ((arena == NULL) || (arena->base == NULL) || (arena->backingSize < 0))
	{
		NativeStateDigest_PutGuestRef(hasher, NATIVE_STATE_DIGEST_GUEST_EXTERNAL, 0);
		return;
	}

	base = (uintptr_t)arena->base;
	address = (uintptr_t)ptr;
	if ((address < base) || ((address - base) > (uintptr_t)(u32)arena->backingSize))
	{
		NativeStateDigest_PutGuestRef(hasher, NATIVE_STATE_DIGEST_GUEST_EXTERNAL, 0);
		return;
	}

	NativeStateDigest_PutGuestRef(hasher, NATIVE_STATE_DIGEST_GUEST_MEMPACK, (u32)(address - base));
}

internal u64 NativeStateDigest_HashTiming(const struct GameTracker *gGT)
{
	struct NativeStateDigestHasher hasher = NativeStateDigest_Begin(NATIVE_STATE_DIGEST_FOURCC('T', 'I', 'M', 'E'));

	NativeStateDigest_PutU32(&hasher, NATIVE_STATE_DIGEST_SCHEMA_VERSION);
	NativeStateDigest_PutS32(&hasher, gGT->frameTimer_VsyncCallback);
	NativeStateDigest_PutS32(&hasher, gGT->frameTimer_notPaused);
	NativeStateDigest_PutS32(&hasher, gGT->timer);
	NativeStateDigest_PutS32(&hasher, gGT->framesInThisLEV);
	NativeStateDigest_PutS32(&hasher, gGT->msInThisLEV);
	NativeStateDigest_PutS32(&hasher, gGT->elapsedTimeMS);
	NativeStateDigest_PutS32(&hasher, gGT->elapsedEventTime);
	NativeStateDigest_PutS32(&hasher, gGT->trafficLightsTimer);
	NativeStateDigest_PutS32(&hasher, gGT->frozenTimeRemaining);
	NativeStateDigest_PutS32(&hasher, gGT->originalEventTime);
	NativeStateDigest_PutS32(&hasher, gGT->clockDurationStall);
	NativeStateDigest_PutU8(&hasher, sdata != NULL);
	if (sdata != NULL)
	{
		NativeStateDigest_PutS32(&hasher, sdata->frameCounter);
	}

	return hasher.value;
}

internal u64 NativeStateDigest_HashRng(const struct GameTracker *gGT)
{
	struct NativeStateDigestHasher hasher = NativeStateDigest_Begin(NATIVE_STATE_DIGEST_FOURCC('R', 'N', 'G', 'S'));

	NativeStateDigest_PutU32(&hasher, NATIVE_STATE_DIGEST_SCHEMA_VERSION);
	NativeStateDigest_PutU32(&hasher, (u32)gGT->deadcoed_struct.state0);
	NativeStateDigest_PutU32(&hasher, (u32)gGT->deadcoed_struct.state1);
	NativeStateDigest_PutU8(&hasher, sdata != NULL);
	if (sdata != NULL)
	{
		NativeStateDigest_PutU32(&hasher, (u32)sdata->randomNumber);
		NativeStateDigest_PutU32(&hasher, sdata->audioRNG);
		NativeStateDigest_PutU32(&hasher, (u32)sdata->const_0x30215400);
		NativeStateDigest_PutU32(&hasher, (u32)sdata->const_0x493583fe);
	}

	return hasher.value;
}

internal void NativeStateDigest_PutDriverPhysicsConstants(struct NativeStateDigestHasher *hasher, const struct Driver *driver)
{
	NativeStateDigest_PutS16(hasher, driver->const_Gravity);
	NativeStateDigest_PutS16(hasher, driver->const_JumpForce);
	NativeStateDigest_PutS16(hasher, driver->const_PedalFriction_Perpendicular);
	NativeStateDigest_PutS16(hasher, driver->const_PedalFriction_Forward);
	NativeStateDigest_PutS16(hasher, driver->const_NoPedalFriction_Perpendicular);
	NativeStateDigest_PutS16(hasher, driver->const_NoPedalFriction_Forward);
	NativeStateDigest_PutS16(hasher, driver->const_BrakeFriction);
	NativeStateDigest_PutS16(hasher, driver->const_DriftCurve);
	NativeStateDigest_PutS16(hasher, driver->const_DriftFriction);
	NativeStateDigest_PutS16(hasher, driver->const_Accel_ClassStat);
	NativeStateDigest_PutS16(hasher, driver->const_Accel_Reserves);
	NativeStateDigest_PutS16(hasher, driver->const_Speed_ClassStat);
	NativeStateDigest_PutS16(hasher, driver->const_AccelSpeed_ClassStat);
	NativeStateDigest_PutS16(hasher, driver->const_SingleTurboSpeed);
	NativeStateDigest_PutS16(hasher, driver->const_SacredFireSpeed);
	NativeStateDigest_PutS16(hasher, driver->const_BackwardSpeed);
	NativeStateDigest_PutS16(hasher, driver->const_MaskSpeed);
	NativeStateDigest_PutS16(hasher, driver->const_DamagedSpeed);
	NativeStateDigest_PutS16(hasher, driver->const_CollisionWeight);
	NativeStateDigest_PutS16(hasher, driver->const_SlopeForwardSpeedBonus);
	NativeStateDigest_PutS16(hasher, driver->const_SideSpeedClamp);
}

internal void NativeStateDigest_PutDriverKartState(struct NativeStateDigestHasher *hasher, const struct Driver *driver)
{
	switch (driver->kartState)
	{
	case KS_DRIFTING:
		NativeStateDigest_PutS16(hasher, driver->KartStates.Drifting.numFramesDrifting);
		NativeStateDigest_PutS16(hasher, driver->KartStates.Drifting.driftBoostTimeMS);
		NativeStateDigest_PutS16(hasher, driver->KartStates.Drifting.driftTotalTimeMS);
		NativeStateDigest_PutU8(hasher, (u8)driver->KartStates.Drifting.numBoostsAttempted);
		NativeStateDigest_PutU8(hasher, (u8)driver->KartStates.Drifting.numBoostsSuccess);
		break;
	case KS_SPINNING:
		NativeStateDigest_PutS16(hasher, driver->KartStates.Spinning.driftSpinRate);
		NativeStateDigest_PutS16(hasher, driver->KartStates.Spinning.spinDir);
		break;
	case KS_ENGINE_REVVING:
		NativeStateDigest_PutS32(hasher, driver->KartStates.RevEngine.boostMeter);
		NativeStateDigest_PutS32(hasher, driver->KartStates.RevEngine.fireLevel);
		NativeStateDigest_PutS16(hasher, driver->KartStates.RevEngine.overRevTimerMS);
		NativeStateDigest_PutS16(hasher, driver->KartStates.RevEngine.releaseCooldownTimerMS);
		NativeStateDigest_PutS16(hasher, driver->KartStates.RevEngine.emptyCooldownTimerMS);
		NativeStateDigest_PutU8(hasher, driver->KartStates.RevEngine.chargeState);
		NativeStateDigest_PutU8(hasher, driver->KartStates.RevEngine.lockoutFlags);
		NativeStateDigest_PutS32(hasher, driver->KartStates.RevEngine.boolMaskGrab);
		break;
	case KS_MASK_GRABBED:
		NativeStateDigest_PutSVec3(hasher, &driver->KartStates.MaskGrab.AngleAxis_NormalVec);
		NativeStateDigest_PutS16(hasher, driver->KartStates.MaskGrab.animFrame);
		NativeStateDigest_PutU8(hasher, driver->KartStates.MaskGrab.boolParticlesSpawned);
		NativeStateDigest_PutU8(hasher, driver->KartStates.MaskGrab.boolStillFalling);
		NativeStateDigest_PutU8(hasher, driver->KartStates.MaskGrab.boolLiftingPlayer);
		NativeStateDigest_PutU8(hasher, driver->KartStates.MaskGrab.boolWhistle);
		break;
	case KS_BLASTED:
		NativeStateDigest_PutU8(hasher, driver->KartStates.Blasted.boolPlayBackwards);
		break;
	case KS_WARP_PAD:
		NativeStateDigest_PutS32(hasher, driver->KartStates.Warp.timer);
		NativeStateDigest_PutS32(hasher, driver->KartStates.Warp.heightOffset);
		NativeStateDigest_PutS32(hasher, driver->KartStates.Warp.quadHeight);
		NativeStateDigest_PutS32(hasher, driver->KartStates.Warp.dustAngle);
		NativeStateDigest_PutS32(hasher, driver->KartStates.Warp.beamHeight);
		break;
	case KS_NORMAL:
	case KS_CRASHING:
	case KS_ANTIVSHIFT:
	case KS_FREEZE:
	default:
		break;
	}
}

internal void NativeStateDigest_PutBotState(struct NativeStateDigestHasher *hasher, const struct Driver *driver)
{
	const struct BotData *bot = &driver->botData;

	NativeStateDigest_PutU32(hasher, bot->botFlags);
	NativeStateDigest_PutS32(hasher, bot->botAccel);
	NativeStateDigest_PutS16(hasher, bot->botPath);
	NativeStateDigest_PutS16(hasher, bot->aiDamageState);
	NativeStateDigest_PutS32(hasher, bot->navProgressRemainder);
	NativeStateDigest_PutS16(hasher, bot->aiPhysics.rotXZ);
	NativeStateDigest_PutS16(hasher, bot->aiPhysics.driftTarget);
	NativeStateDigest_PutS16(hasher, bot->aiPhysics.mulDrift);
	NativeStateDigest_PutS16(hasher, bot->aiPhysics.simpTurnState);
	NativeStateDigest_PutS16(hasher, bot->aiPhysics.turboMeter);
	NativeStateDigest_PutS16(hasher, bot->aiPhysics.fireLevel);
	NativeStateDigest_PutS32(hasher, bot->aiPhysics.squishCooldown);
	NativeStateDigest_PutS32(hasher, bot->aiPhysics.speedY);
	NativeStateDigest_PutS32(hasher, bot->aiPhysics.speedLinear);
	NativeStateDigest_PutVec3(hasher, &bot->aiPhysics.accel);
	NativeStateDigest_PutVec3(hasher, &bot->aiPhysics.velocity);
	NativeStateDigest_PutVec3(hasher, &bot->positionBackup);
	NativeStateDigest_PutSVec3(hasher, &bot->aiRot);
	NativeStateDigest_PutS32(hasher, bot->ai_progress_cooldown);
	NativeStateDigest_PutS16(hasher, bot->ai_rotY_608);
	NativeStateDigest_PutU8(hasher, bot->ai_quadblock_checkpointIndex);
	NativeStateDigest_PutSVec3(hasher, &bot->estimatePosition);
	NativeStateDigest_PutU8(hasher, bot->estimateRotNav[0]);
	NativeStateDigest_PutU8(hasher, bot->estimateRotNav[1]);
	NativeStateDigest_PutU8(hasher, bot->estimateRotNav[2]);
	NativeStateDigest_PutU8(hasher, bot->estimateRotCurrY);
	NativeStateDigest_PutS16(hasher, bot->distToNextNavXYZ);
	NativeStateDigest_PutS16(hasher, bot->distToNextNavXZ);
	NativeStateDigest_PutS16(hasher, bot->estimateFlags);
	NativeStateDigest_PutS16(hasher, bot->weaponCooldown);
	NativeStateDigest_PutU8(hasher, bot->blastBounceCount);
	NativeStateDigest_PutU8(hasher, bot->desiredPath_BossOnly);
}

internal void NativeStateDigest_PutDriver(struct NativeStateDigestHasher *hasher, const struct GameTracker *gGT, const struct Driver *driver)
{
	NativeStateDigest_PutU8(hasher, driver->driverID);
	NativeStateDigest_PutU8(hasher, driver->kartState);
	NativeStateDigest_PutU32(hasher, driver->actionsFlagSet);
	NativeStateDigest_PutU32(hasher, driver->actionsFlagSetPrevFrame);
	NativeStateDigest_PutS16(hasher, driver->collisionFlags);
	NativeStateDigest_PutU32(hasher, driver->stepFlagSet);
	NativeStateDigest_PutVec3(hasher, &driver->posCurr);
	NativeStateDigest_PutVec3(hasher, &driver->posPrev);
	NativeStateDigest_PutVec3(hasher, &driver->velocity);
	NativeStateDigest_PutSVec3Slot(hasher, &driver->rotCurr);
	NativeStateDigest_PutSVec3Slot(hasher, &driver->rotPrev);
	NativeStateDigest_PutSVec3(hasher, &driver->normalVecUP);
	NativeStateDigest_PutSVec3(hasher, &driver->spsHitPos);
	NativeStateDigest_PutSVec3(hasher, &driver->spsNormalVec);
	NativeStateDigest_PutS32(hasher, driver->quadBlockHeight);
	NativeStateDigest_PutS16(hasher, driver->speed);
	NativeStateDigest_PutS16(hasher, driver->speedApprox);
	NativeStateDigest_PutS32(hasher, driver->xSpeed);
	NativeStateDigest_PutS32(hasher, driver->ySpeed);
	NativeStateDigest_PutS32(hasher, driver->zSpeed);
	NativeStateDigest_PutSVec3(hasher, &driver->forwardAccelVector);
	NativeStateDigest_PutS16(hasher, driver->forwardAccelImpulse);
	NativeStateDigest_PutSVec3(hasher, &driver->accel);
	NativeStateDigest_PutS16(hasher, driver->turnAngleCurr);
	NativeStateDigest_PutS16(hasher, driver->turnAnglePrev);
	NativeStateDigest_PutS16(hasher, driver->turnAngleLerpTarget);
	NativeStateDigest_PutS16(hasher, driver->turnAngleLerpVel);
	NativeStateDigest_PutS16(hasher, driver->multDrift);
	NativeStateDigest_PutS16(hasher, driver->previousFrameMultDrift);
	NativeStateDigest_PutS16(hasher, driver->turbo_MeterRoomLeft);
	NativeStateDigest_PutS16(hasher, driver->turbo_outsideTimer);
	NativeStateDigest_PutS16(hasher, driver->reserves);
	NativeStateDigest_PutS16(hasher, driver->fireSpeed);
	NativeStateDigest_PutS16(hasher, driver->fireSpeedCap);
	NativeStateDigest_PutS16(hasher, driver->baseSpeed);
	NativeStateDigest_PutS16(hasher, driver->terrainScaledBaseSpeed);
	NativeStateDigest_PutS16(hasher, driver->jumpHeightCurr);
	NativeStateDigest_PutS16(hasher, driver->jumpHeightPrev);
	NativeStateDigest_PutS16(hasher, driver->jump_TenBuffer);
	NativeStateDigest_PutS16(hasher, driver->jump_CooldownMS);
	NativeStateDigest_PutS16(hasher, driver->jump_CoyoteTimerMS);
	NativeStateDigest_PutS16(hasher, driver->jump_ForcedMS);
	NativeStateDigest_PutS16(hasher, driver->jump_InitialVelY);
	NativeStateDigest_PutS16(hasher, driver->jump_HighJumpTimerMS);
	NativeStateDigest_PutS16(hasher, driver->jump_LandingBoost);
	NativeStateDigest_PutS16(hasher, driver->wallRubTimer);
	NativeStateDigest_PutS16(hasher, driver->terrainFrictionTimer);
	NativeStateDigest_PutU8(hasher, driver->currentTerrain);
	NativeStateDigest_PutU8(hasher, driver->forcedJumpType);
	NativeStateDigest_PutS32(hasher, driver->lapTime);
	NativeStateDigest_PutU8(hasher, driver->lapIndex);
	NativeStateDigest_PutS16(hasher, driver->driverRank);
	NativeStateDigest_PutU32(hasher, driver->distanceToFinish_curr);
	NativeStateDigest_PutU32(hasher, driver->distanceToFinish_checkpoint);
	NativeStateDigest_PutU32(hasher, driver->distanceDrivenBackwards);
	NativeStateDigest_PutU8(hasher, driver->checkpoint.branchChoiceIndex);
	NativeStateDigest_PutU8(hasher, driver->checkpoint.currentIndex);
	NativeStateDigest_PutU8(hasher, driver->heldItemID);
	NativeStateDigest_PutU8(hasher, driver->numHeldItems);
	NativeStateDigest_PutS16(hasher, driver->itemRollTimer);
	NativeStateDigest_PutS16(hasher, driver->noItemTimer);
	NativeStateDigest_PutU8(hasher, (u8)driver->numWumpas);
	NativeStateDigest_PutU8(hasher, (u8)driver->numCrystals);
	NativeStateDigest_PutU8(hasher, (u8)driver->numTimeCrates);
	NativeStateDigest_PutS32(hasher, driver->invincibleTimer);
	NativeStateDigest_PutS32(hasher, driver->invisibleTimer);
	NativeStateDigest_PutU8(hasher, driver->pendingDamageType);
	NativeStateDigest_PutU8(hasher, driver->pendingDamageReasonByte);
	NativeStateDigest_PutDriverRef(hasher, gGT, driver->pendingDamageAttacker);
	NativeStateDigest_PutQuadBlockRef(hasher, gGT, driver->currBlockTouching);
	NativeStateDigest_PutQuadBlockRef(hasher, gGT, driver->underDriver);
	NativeStateDigest_PutQuadBlockRef(hasher, gGT, driver->lastValid);
	NativeStateDigest_PutDriverPhysicsConstants(hasher, driver);
	NativeStateDigest_PutDriverKartState(hasher, driver);
	NativeStateDigest_PutU8(hasher, (driver->actionsFlagSet & ACTION_BOT) != 0);
	if ((driver->actionsFlagSet & ACTION_BOT) != 0)
	{
		NativeStateDigest_PutBotState(hasher, driver);
	}
}

internal u64 NativeStateDigest_HashDrivers(const struct GameTracker *gGT)
{
	struct NativeStateDigestHasher hasher = NativeStateDigest_Begin(NATIVE_STATE_DIGEST_FOURCC('D', 'R', 'V', 'R'));

	NativeStateDigest_PutU32(&hasher, NATIVE_STATE_DIGEST_SCHEMA_VERSION);
	for (u32 i = 0; i < len(gGT->drivers); i++)
	{
		const struct Driver *driver = gGT->drivers[i];

		NativeStateDigest_PutU8(&hasher, (u8)i);
		NativeStateDigest_PutU8(&hasher, driver != NULL);
		if (driver != NULL)
		{
			NativeStateDigest_PutDriver(&hasher, gGT, driver);
		}
	}

	return hasher.value;
}

internal u64 NativeStateDigest_HashWorld(const struct GameTracker *gGT)
{
	struct NativeStateDigestHasher hasher = NativeStateDigest_Begin(NATIVE_STATE_DIGEST_FOURCC('W', 'R', 'L', 'D'));

	NativeStateDigest_PutU32(&hasher, NATIVE_STATE_DIGEST_SCHEMA_VERSION);
	NativeStateDigest_PutS32(&hasher, gGT->gameMode1);
	NativeStateDigest_PutS32(&hasher, gGT->gameMode2);
	NativeStateDigest_PutS32(&hasher, gGT->levelID);
	NativeStateDigest_PutS32(&hasher, gGT->currLEV);
	NativeStateDigest_PutS32(&hasher, gGT->prevLEV);
	NativeStateDigest_PutU8(&hasher, gGT->numPlyrCurrGame);
	NativeStateDigest_PutU8(&hasher, gGT->numPlyrNextGame);
	NativeStateDigest_PutU8(&hasher, gGT->numBotsCurrGame);
	NativeStateDigest_PutU8(&hasher, gGT->numBotsNextGame);
	NativeStateDigest_PutU8(&hasher, (u8)gGT->numLaps);
	NativeStateDigest_PutS32(&hasher, gGT->numParticles);
	NativeStateDigest_PutS32(&hasher, gGT->numCrystalsInLEV);
	NativeStateDigest_PutS32(&hasher, gGT->timeCratesInLEV);
	for (u32 i = 0; i < len(gGT->lapTime); i++)
	{
		NativeStateDigest_PutS32(&hasher, gGT->lapTime[i]);
	}
	NativeStateDigest_PutS32(&hasher, gGT->bestLapTime);
	NativeStateDigest_PutS32(&hasher, gGT->lapIndexNewBest);
	NativeStateDigest_PutU32(&hasher, gGT->gameModeEnd);
	NativeStateDigest_PutS32(&hasher, gGT->currAdvProfile.numTrophies);
	NativeStateDigest_PutS32(&hasher, gGT->currAdvProfile.numRelics);
	NativeStateDigest_PutS32(&hasher, gGT->currAdvProfile.numKeys);
	NativeStateDigest_PutS32(&hasher, gGT->currAdvProfile.numCtrTokens.total);
	NativeStateDigest_PutS32(&hasher, gGT->currAdvProfile.numCtrTokens.red);
	NativeStateDigest_PutS32(&hasher, gGT->currAdvProfile.numCtrTokens.green);
	NativeStateDigest_PutS32(&hasher, gGT->currAdvProfile.numCtrTokens.blue);
	NativeStateDigest_PutS32(&hasher, gGT->currAdvProfile.numCtrTokens.yellow);
	NativeStateDigest_PutS32(&hasher, gGT->currAdvProfile.numCtrTokens.purple);
	NativeStateDigest_PutS32(&hasher, gGT->currAdvProfile.completionPercent);
	NativeStateDigest_PutS32(&hasher, gGT->cup.cupID);
	NativeStateDigest_PutS32(&hasher, gGT->cup.trackIndex);
	for (u32 i = 0; i < len(gGT->cup.points); i++)
	{
		NativeStateDigest_PutS32(&hasher, gGT->cup.points[i]);
	}
	NativeStateDigest_PutS32(&hasher, gGT->battleSetup.lifeLimit);
	NativeStateDigest_PutS32(&hasher, gGT->battleSetup.killLimit);
	for (u32 i = 0; i < len(gGT->battleSetup.pointsPerTeam); i++)
	{
		NativeStateDigest_PutS32(&hasher, gGT->battleSetup.pointsPerTeam[i]);
		NativeStateDigest_PutU8(&hasher, (u8)gGT->battleSetup.teamOfEachPlayer[i]);
	}
	NativeStateDigest_PutU32(&hasher, gGT->battleSetup.teamFlags);
	NativeStateDigest_PutS32(&hasher, gGT->battleSetup.numTeams);
	NativeStateDigest_PutU8(&hasher, sdata != NULL);
	if (sdata != NULL)
	{
		NativeStateDigest_PutS32(&hasher, sdata->mainGameState);
		NativeStateDigest_PutS32(&hasher, sdata->Loading.stage);
		NativeStateDigest_PutS32(&hasher, sdata->mainMenuState);
	}

	return hasher.value;
}

internal void NativeStateDigest_PutJitPool(struct NativeStateDigestHasher *hasher, const struct JitPool *pool)
{
	NativeStateDigest_PutS32(hasher, pool->free.count);
	NativeStateDigest_PutS32(hasher, pool->taken.count);
	NativeStateDigest_PutS32(hasher, pool->maxItems);
	NativeStateDigest_PutU32(hasher, pool->itemSize);
	NativeStateDigest_PutS32(hasher, pool->poolSize);
}

internal void NativeStateDigest_PutMempack(struct NativeStateDigestHasher *hasher, const struct Mempack *mempack)
{
	NativeStateDigest_PutS32(hasher, mempack->packSize);
	NativeStateDigest_PutMempackRef(hasher, mempack->start);
	NativeStateDigest_PutMempackRef(hasher, mempack->lastFreeByte);
	NativeStateDigest_PutMempackRef(hasher, mempack->endOfAllocator);
	NativeStateDigest_PutMempackRef(hasher, mempack->endOfMemory);
	NativeStateDigest_PutMempackRef(hasher, mempack->firstFreeByte);
	NativeStateDigest_PutS32(hasher, mempack->sizeOfPrevAllocation);
	NativeStateDigest_PutS32(hasher, mempack->numBookmarks);
	for (u32 i = 0; i < len(mempack->bookmarks); i++)
	{
		NativeStateDigest_PutMempackRef(hasher, mempack->bookmarks[i]);
	}
}

internal u64 NativeStateDigest_HashAllocation(const struct GameTracker *gGT)
{
	const struct JitPool *pools[] = {
	    &gGT->JitPools.thread,     &gGT->JitPools.instance, &gGT->JitPools.smallStack, &gGT->JitPools.mediumStack,
	    &gGT->JitPools.largeStack, &gGT->JitPools.particle, &gGT->JitPools.oscillator, &gGT->JitPools.rain,
	};
	struct NativeStateDigestHasher hasher = NativeStateDigest_Begin(NATIVE_STATE_DIGEST_FOURCC('A', 'L', 'O', 'C'));

	NativeStateDigest_PutU32(&hasher, NATIVE_STATE_DIGEST_SCHEMA_VERSION);
	for (u32 i = 0; i < len(pools); i++)
	{
		NativeStateDigest_PutU8(&hasher, (u8)i);
		NativeStateDigest_PutJitPool(&hasher, pools[i]);
	}
	NativeStateDigest_PutU8(&hasher, sdata != NULL);
	if (sdata != NULL)
	{
		NativeStateDigest_PutS32(&hasher, gGT->activeMempackIndex);
		for (u32 i = 0; i < len(sdata->mempack); i++)
		{
			NativeStateDigest_PutU8(&hasher, (u8)i);
			NativeStateDigest_PutMempack(&hasher, &sdata->mempack[i]);
		}
	}

	return hasher.value;
}

void NativeStateDigest_Capture(const struct GameTracker *gGT, struct NativeStateDigest *out)
{
	struct NativeStateDigestHasher root;

	if (out == NULL)
	{
		return;
	}

	memset(out, 0, sizeof(*out));
	out->schemaVersion = NATIVE_STATE_DIGEST_SCHEMA_VERSION;
	out->componentMask = NATIVE_STATE_DIGEST_COMPONENT_ALL;
	if (gGT == NULL)
	{
		return;
	}

	out->timing = NativeStateDigest_HashTiming(gGT);
	out->rng = NativeStateDigest_HashRng(gGT);
	out->drivers = NativeStateDigest_HashDrivers(gGT);
	out->world = NativeStateDigest_HashWorld(gGT);
	out->allocation = NativeStateDigest_HashAllocation(gGT);

	root = NativeStateDigest_Begin(NATIVE_STATE_DIGEST_FOURCC('R', 'O', 'O', 'T'));
	NativeStateDigest_PutU32(&root, out->schemaVersion);
	NativeStateDigest_PutU32(&root, out->componentMask);
	NativeStateDigest_PutU64(&root, out->timing);
	NativeStateDigest_PutU64(&root, out->rng);
	NativeStateDigest_PutU64(&root, out->drivers);
	NativeStateDigest_PutU64(&root, out->world);
	NativeStateDigest_PutU64(&root, out->allocation);
	out->root = root.value;
}

u32 NativeStateDigest_DifferenceMask(const struct NativeStateDigest *expected, const struct NativeStateDigest *live)
{
	u32 difference = 0;

	if ((expected == NULL) || (live == NULL) || (expected->schemaVersion != live->schemaVersion) ||
	    (expected->componentMask != live->componentMask))
	{
		return NATIVE_STATE_DIGEST_DIFFERENCE_SCHEMA;
	}
	if (expected->timing != live->timing)
	{
		difference |= NATIVE_STATE_DIGEST_COMPONENT_TIMING;
	}
	if (expected->rng != live->rng)
	{
		difference |= NATIVE_STATE_DIGEST_COMPONENT_RNG;
	}
	if (expected->drivers != live->drivers)
	{
		difference |= NATIVE_STATE_DIGEST_COMPONENT_DRIVERS;
	}
	if (expected->world != live->world)
	{
		difference |= NATIVE_STATE_DIGEST_COMPONENT_WORLD;
	}
	if (expected->allocation != live->allocation)
	{
		difference |= NATIVE_STATE_DIGEST_COMPONENT_ALLOCATION;
	}

	return difference;
}

const char *NativeStateDigest_FirstDifferenceName(u32 differenceMask)
{
	if ((differenceMask & NATIVE_STATE_DIGEST_DIFFERENCE_SCHEMA) != 0)
	{
		return "schema";
	}
	if ((differenceMask & NATIVE_STATE_DIGEST_COMPONENT_TIMING) != 0)
	{
		return "timing";
	}
	if ((differenceMask & NATIVE_STATE_DIGEST_COMPONENT_RNG) != 0)
	{
		return "rng";
	}
	if ((differenceMask & NATIVE_STATE_DIGEST_COMPONENT_DRIVERS) != 0)
	{
		return "drivers";
	}
	if ((differenceMask & NATIVE_STATE_DIGEST_COMPONENT_WORLD) != 0)
	{
		return "world";
	}
	if ((differenceMask & NATIVE_STATE_DIGEST_COMPONENT_ALLOCATION) != 0)
	{
		return "allocation";
	}

	return "none";
}

int NativeStateDigest_RunSelfTest(void)
{
	struct GameTracker trackerA;
	struct GameTracker trackerB;
	struct Driver driverA;
	struct Driver driverB;
	struct NativeStateDigest digestA;
	struct NativeStateDigest digestB;
	struct NativeStateDigest mutated;
	u32 difference;

	memset(&trackerA, 0, sizeof(trackerA));
	memset(&trackerB, 0, sizeof(trackerB));
	memset(&driverA, 0, sizeof(driverA));
	memset(&driverB, 0, sizeof(driverB));

	trackerA.drivers[0] = &driverA;
	trackerB.drivers[0] = &driverB;
	trackerA.backBuffer = &trackerA.db[0];
	trackerB.backBuffer = &trackerB.db[1];
	trackerA.frontBuffer = &trackerA.db[1];
	trackerB.frontBuffer = &trackerB.db[0];
	trackerA.clockFrameStart = 111;
	trackerB.clockFrameStart = 999999;
	trackerA.final_filler_mostly_null[0] = 0x11;
	trackerB.final_filler_mostly_null[0] = 0x77;

	driverA.driverID = 3;
	driverB.driverID = 3;
	driverA.posCurr.x = 0x123456;
	driverB.posCurr.x = 0x123456;
	driverA.posCurr.y = -0x2345;
	driverB.posCurr.y = -0x2345;
	driverA.velocity.z = 0x34567;
	driverB.velocity.z = 0x34567;
	driverA.reserves = 0x456;
	driverB.reserves = 0x456;
	driverA.lapIndex = 1;
	driverB.lapIndex = 1;
	driverA.heldItemID = HELD_ITEM_MISSILE_1X;
	driverB.heldItemID = HELD_ITEM_MISSILE_1X;
	driverA.pendingDamageAttacker = &driverA;
	driverB.pendingDamageAttacker = &driverB;

	NativeStateDigest_Capture(&trackerA, &digestA);
	NativeStateDigest_Capture(&trackerB, &digestB);
	difference = NativeStateDigest_DifferenceMask(&digestA, &digestB);
	if ((difference != 0) || (digestA.root != digestB.root))
	{
		fprintf(stderr, "[CTR StateDigest] self-test failed: host addresses/padding/wall clock changed component=%s\n",
		        NativeStateDigest_FirstDifferenceName(difference));
		return 1;
	}

	driverB.posCurr.x++;
	NativeStateDigest_Capture(&trackerB, &mutated);
	difference = NativeStateDigest_DifferenceMask(&digestA, &mutated);
	if ((difference != NATIVE_STATE_DIGEST_COMPONENT_DRIVERS) || (digestA.root == mutated.root))
	{
		fprintf(stderr, "[CTR StateDigest] self-test failed: position mutation changed component=%s mask=0x%08x\n",
		        NativeStateDigest_FirstDifferenceName(difference), difference);
		return 1;
	}

	printf("[CTR StateDigest] self-test passed: address-independent root=%08x%08x mutation component=%s\n", (u32)(digestA.root >> 32),
	       (u32)digestA.root, NativeStateDigest_FirstDifferenceName(difference));
	return 0;
}

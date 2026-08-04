#include <common.h>

enum LibraryOfModelsConstants
{
	LIBRARY_OF_MODELS_CLEAR_COUNT = 0xe2,
};

internal int LibraryOfModels_IdIsValid(s16 modelID)
{
	return (modelID >= 0) && (modelID < LIBRARY_OF_MODELS_CLEAR_COUNT);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003147c-0x800314c0.
void LibraryOfModels_Store(struct GameTracker *gGT, u32 numModels, struct CtrAssetRef32 *ptrModelArray)
{
	u32 modelIndex = 0;

	// MPK model arrays are null-terminated and retail passes UINT32_MAX. Do not
	// let a missing or damaged terminator walk beyond the number of model slots
	// the game can actually populate.
	if (numModels == UINT32_MAX)
	{
		numModels = LIBRARY_OF_MODELS_CLEAR_COUNT;
	}

#if defined(SDL_PLATFORM_VISIONOS)
	Platform_Log("[CTR Load] model list begin refs=%p limit=%u\n", (void *)ptrModelArray, numModels);
#endif

	while (numModels != 0)
	{
		struct Model *m = NULL;
		if (!CtrAssetRef_ResolveOptional(*ptrModelArray, sizeof(*m), _Alignof(struct Model), (void **)&m,
						"LibraryOfModels_Store model") ||
		    (m == NULL))
		{
#if defined(SDL_PLATFORM_VISIONOS)
			Platform_Log("[CTR Load] model list end index=%u ref=0x%08x reason=%s\n", modelIndex,
			             ptrModelArray->bits, ptrModelArray->bits == 0 ? "terminator" : "invalid-reference");
#endif
			return;
		}
		if (LibraryOfModels_IdIsValid(m->id))
		{
			gGT->modelPtr[m->id] = m;
		}
		else if (m->id != -1)
		{
#if defined(SDL_PLATFORM_VISIONOS)
			Platform_Log("[CTR Load] ignored model index=%u id=%d outside [0,%d)\n", modelIndex,
			             m->id, LIBRARY_OF_MODELS_CLEAR_COUNT);
#endif
		}
		numModels--;
		ptrModelArray++;
		modelIndex++;
	}

#if defined(SDL_PLATFORM_VISIONOS)
	Platform_Log("[CTR Load] model list reached safety limit count=%u\n", modelIndex);
#endif
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800314c0-0x800314e0.
void LibraryOfModels_Clear(struct GameTracker *gGT)
{
	for (s32 i = 0; i < LIBRARY_OF_MODELS_CLEAR_COUNT; i++)
	{
		gGT->modelPtr[i] = 0;
	}
}

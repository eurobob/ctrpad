#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031b50-0x80031bdc.
void LOAD_GlobalModelPtrs_MPK()
{
	struct GameTracker *gGT = sdata->gGT;

#if defined(SDL_PLATFORM_VISIONOS)
	Platform_Log("[CTR Load] MPK global models begin extras=%d refs=%p\n", LOAD_DRIVER_MODEL_EXTRA_COUNT,
	             (void *)sdata->PLYROBJECTLIST);
#endif

	for (int i = 0; i < LOAD_DRIVER_MODEL_EXTRA_COUNT; i++)
	{
		struct Model *m = data.driverModelExtras[i].model;

		if (m == NULL)
		{
			continue;
		}

		if (!LibraryOfModels_IdIsValid(m->id))
		{
			if (m->id != -1)
			{
#if defined(SDL_PLATFORM_VISIONOS)
				Platform_Log("[CTR Load] ignored driver extra=%d id=%d outside model table\n", i, m->id);
#endif
			}
			continue;
		}

		gGT->modelPtr[m->id] = m;
	}

#if defined(SDL_PLATFORM_VISIONOS)
	Platform_Log("[CTR Load] MPK driver extras complete\n");
#endif

	if (sdata->PLYROBJECTLIST != 0)
	{
		LibraryOfModels_Store(gGT, UINT32_MAX, sdata->PLYROBJECTLIST);
	}

#if defined(SDL_PLATFORM_VISIONOS)
	Platform_Log("[CTR Load] MPK global models complete\n");
#endif
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031bdc-0x80031c1c.
void LOAD_HubSwapPtrs(struct GameTracker *gGT)
{
	struct Level *oldLev1;
	struct VisMem *oldVisMem1;
	struct VisMem *oldVisMem2;

	// if no secondary lev exists, quit
	if (gGT->level2 == 0)
	{
		return;
	}

	oldLev1 = gGT->level1;
	oldVisMem1 = gGT->visMem1;
	oldVisMem2 = gGT->visMem2;

	gGT->level1 = gGT->level2;
	gGT->boolHubSwapped = 1;

	gGT->level2 = oldLev1;
	gGT->visMem1 = oldVisMem2;
	gGT->visMem2 = oldVisMem1;
}

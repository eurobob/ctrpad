#include "platform/native_storage.h"

#include <macros.h>

#include "platform/native_path.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <string.h>

#define NATIVE_STORAGE_PATH_MAX        1024
#define NATIVE_STORAGE_ORGANIZATION    "chrissotraidis"
#define NATIVE_STORAGE_APPLICATION     "CTRPad"
#define NATIVE_STORAGE_IMPORT_DIR_NAME "CTRPad"
#define NATIVE_STORAGE_ASSET_DIR_NAME  "assets"

struct NativeStoragePaths
{
	char executableBase[NATIVE_STORAGE_PATH_MAX];
	char writableRoot[NATIVE_STORAGE_PATH_MAX];
	char documentsRoot[NATIVE_STORAGE_PATH_MAX];
	char importBaseDir[NATIVE_STORAGE_PATH_MAX];
	char importAssetDir[NATIVE_STORAGE_PATH_MAX];
	int sandboxed;
	int initialized;
};

global_variable struct NativeStoragePaths s_nativeStorage;

internal int NativeStorage_CopyNormalized(char *dst, size_t dstSize, const char *src)
{
	NativeStr8 path;

	if ((dst == NULL) || (dstSize == 0) || (src == NULL) || (src[0] == '\0'))
	{
		return 0;
	}

	path = NativePath_TrimTrailingSeparators(NativeStr8_FromCString(src));
	return NativePath_NormalizeSlashes(dst, dstSize, path);
}

internal int NativeStorage_Configure(struct NativeStoragePaths *paths, const char *executableBasePath, const char *writableRoot,
                                    const char *documentsRoot, int sandboxed)
{
	if ((paths == NULL) || !NativeStorage_CopyNormalized(paths->executableBase, sizeof(paths->executableBase), executableBasePath) ||
	    !NativeStorage_CopyNormalized(paths->writableRoot, sizeof(paths->writableRoot), writableRoot) ||
	    !NativeStorage_CopyNormalized(paths->documentsRoot, sizeof(paths->documentsRoot), documentsRoot))
	{
		return 0;
	}

	if (!NativePath_Join(paths->importBaseDir, sizeof(paths->importBaseDir), NativeStr8_FromCString(paths->documentsRoot),
	                     NATIVE_STR8_LIT(NATIVE_STORAGE_IMPORT_DIR_NAME)) ||
	    !NativePath_Join(paths->importAssetDir, sizeof(paths->importAssetDir), NativeStr8_FromCString(paths->importBaseDir),
	                     NATIVE_STR8_LIT(NATIVE_STORAGE_ASSET_DIR_NAME)))
	{
		return 0;
	}

	paths->sandboxed = sandboxed != 0;
	paths->initialized = 1;
	return 1;
}

int NativeStorage_Init(const char *executableBasePath)
{
	const char *basePath = executableBasePath;

	memset(&s_nativeStorage, 0, sizeof(s_nativeStorage));
	if ((basePath == NULL) || (basePath[0] == '\0'))
	{
		basePath = ".";
	}

#if defined(SDL_PLATFORM_IOS) || (defined(__APPLE__) && defined(CTR_NATIVE_MACOS_BUNDLE))
	{
		char *preferencePath = SDL_GetPrefPath(NATIVE_STORAGE_ORGANIZATION, NATIVE_STORAGE_APPLICATION);
	#if defined(SDL_PLATFORM_IOS)
		const char *documentsPath = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
	#else
		const char *documentsPath = preferencePath;
	#endif
		int ok;

		if ((preferencePath == NULL) || (documentsPath == NULL))
		{
			fprintf(stderr, "[CTR Storage] failed to resolve app storage paths: %s\n", SDL_GetError());
			SDL_free(preferencePath);
			return 0;
		}

		ok = NativeStorage_Configure(&s_nativeStorage, basePath, preferencePath, documentsPath, 1);
		SDL_free(preferencePath);
		if (!ok)
		{
			fprintf(stderr, "[CTR Storage] app storage path exceeds %d bytes\n", NATIVE_STORAGE_PATH_MAX - 1);
			return 0;
		}

		if (!SDL_CreateDirectory(s_nativeStorage.writableRoot) || !SDL_CreateDirectory(s_nativeStorage.importAssetDir))
		{
			fprintf(stderr, "[CTR Storage] failed to create app storage directories: %s\n", SDL_GetError());
			memset(&s_nativeStorage, 0, sizeof(s_nativeStorage));
			return 0;
		}
	}
#else
	if (!NativeStorage_Configure(&s_nativeStorage, basePath, basePath, basePath, 0))
	{
		fprintf(stderr, "[CTR Storage] executable path exceeds %d bytes\n", NATIVE_STORAGE_PATH_MAX - 1);
		return 0;
	}
#endif

	return 1;
}

int NativeStorage_FinalizeForAssetBase(const char *assetBasePath)
{
	if (!s_nativeStorage.initialized || (assetBasePath == NULL) || (assetBasePath[0] == '\0'))
	{
		return 0;
	}

	if (s_nativeStorage.sandboxed)
	{
		return 1;
	}

	return NativeStorage_Configure(&s_nativeStorage, assetBasePath, assetBasePath, assetBasePath, 0);
}

const char *NativeStorage_GetWritableRoot(void)
{
	return s_nativeStorage.initialized ? s_nativeStorage.writableRoot : NULL;
}

const char *NativeStorage_GetDocumentsRoot(void)
{
	return s_nativeStorage.initialized ? s_nativeStorage.documentsRoot : NULL;
}

const char *NativeStorage_GetImportBaseDir(void)
{
	if (!s_nativeStorage.initialized || !s_nativeStorage.sandboxed)
	{
		return NULL;
	}

	return s_nativeStorage.importBaseDir;
}

const char *NativeStorage_GetImportAssetDir(void)
{
	if (!s_nativeStorage.initialized || !s_nativeStorage.sandboxed)
	{
		return NULL;
	}

	return s_nativeStorage.importAssetDir;
}

int NativeStorage_BuildWritablePath(const char *relativePath, char *dst, size_t dstSize)
{
	if (!s_nativeStorage.initialized || (relativePath == NULL))
	{
		return 0;
	}

	return NativePath_Join(dst, dstSize, NativeStr8_FromCString(s_nativeStorage.writableRoot), NativeStr8_FromCString(relativePath));
}

int NativeStorage_RunSelfTest(void)
{
	struct NativeStoragePaths sandboxPaths;
	struct NativeStoragePaths portablePaths;
	char writablePath[NATIVE_STORAGE_PATH_MAX];

	memset(&sandboxPaths, 0, sizeof(sandboxPaths));
	memset(&portablePaths, 0, sizeof(portablePaths));

	if (!NativeStorage_Configure(&sandboxPaths, "/Bundle/CTRPad.app/", "/Data/Library/Application Support/CTRPad/",
	                             "/Data/Documents/", 1) ||
	    (strcmp(sandboxPaths.executableBase, "/Bundle/CTRPad.app") != 0) ||
	    (strcmp(sandboxPaths.writableRoot, "/Data/Library/Application Support/CTRPad") != 0) ||
	    (strcmp(sandboxPaths.documentsRoot, "/Data/Documents") != 0) ||
	    (strcmp(sandboxPaths.importBaseDir, "/Data/Documents/CTRPad") != 0) ||
	    (strcmp(sandboxPaths.importAssetDir, "/Data/Documents/CTRPad/assets") != 0) || !sandboxPaths.sandboxed)
	{
		fprintf(stderr, "[CTR Storage] self-test failed: sandbox contract\n");
		return 1;
	}

	s_nativeStorage = sandboxPaths;
	if (!NativeStorage_BuildWritablePath("memcards", writablePath, sizeof(writablePath)) ||
	    (strcmp(writablePath, "/Data/Library/Application Support/CTRPad/memcards") != 0))
	{
		fprintf(stderr, "[CTR Storage] self-test failed: private writable path\n");
		return 1;
	}

	if (!NativeStorage_Configure(&portablePaths, "C:\\Games\\CTR\\", "C:\\Games\\CTR\\", "C:\\Games\\CTR\\", 0) ||
	    (strcmp(portablePaths.writableRoot, "C:/Games/CTR") != 0) || portablePaths.sandboxed)
	{
		fprintf(stderr, "[CTR Storage] self-test failed: portable contract\n");
		return 1;
	}

	memset(&s_nativeStorage, 0, sizeof(s_nativeStorage));
	printf("[CTR Storage] self-test passed: bundle=read-only preferences=private documents=user-visible imports=preferred desktop=portable\n");
	return 0;
}

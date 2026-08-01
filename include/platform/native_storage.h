#ifndef PLATFORM_NATIVE_STORAGE_H
#define PLATFORM_NATIVE_STORAGE_H

#include <stddef.h>

int NativeStorage_Init(const char *executableBasePath);
int NativeStorage_FinalizeForAssetBase(const char *assetBasePath);
const char *NativeStorage_GetWritableRoot(void);
const char *NativeStorage_GetDocumentsRoot(void);
const char *NativeStorage_GetImportBaseDir(void);
const char *NativeStorage_GetImportAssetDir(void);
int NativeStorage_BuildWritablePath(const char *relativePath, char *dst, size_t dstSize);
int NativeStorage_RunSelfTest(void);

#endif

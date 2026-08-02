#ifndef PLATFORM_NATIVE_MACOS_IMPORT_H
#define PLATFORM_NATIVE_MACOS_IMPORT_H

#include <stddef.h>

int NativeMacOSImport_GetRememberedDiscPath(char *dst, size_t dstSize);
int NativeMacOSImport_ChooseDiscPath(char *dst, size_t dstSize);
void NativeMacOSImport_RememberDiscPath(const char *path);
void NativeMacOSImport_ForgetDiscPath(void);
void NativeMacOSImport_ShowInvalidDiscAlert(void);

#endif

#ifndef PLATFORM_NATIVE_IOS_IMPORT_H
#define PLATFORM_NATIVE_IOS_IMPORT_H

#include <stddef.h>

enum NativeIOSImportValidationResult
{
	NATIVE_IOS_IMPORT_VALID = 0,
	NATIVE_IOS_IMPORT_INVALID_FORMAT,
	NATIVE_IOS_IMPORT_WRONG_REGION,
	NATIVE_IOS_IMPORT_INCOMPLETE,
};

typedef enum NativeIOSImportValidationResult (*NativeIOSImportValidateCallback)(const char *stagingBasePath, char *detail,
	                                                                            size_t detailSize, void *userdata);
typedef int (*NativeIOSImportCompletionCallback)(void *userdata);

int NativeIOSImport_Begin(const char *importBaseDir, NativeIOSImportValidateCallback validateCallback,
	                      NativeIOSImportCompletionCallback completionCallback, void *userdata);

#endif

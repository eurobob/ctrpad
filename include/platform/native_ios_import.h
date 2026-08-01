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

enum NativeIOSImportPurpose
{
	NATIVE_IOS_IMPORT_INITIAL_SETUP = 0,
	NATIVE_IOS_IMPORT_RESELECTION,
};

enum NativeIOSImportCompletionResult
{
	NATIVE_IOS_IMPORT_COMPLETION_FAILED = 0,
	NATIVE_IOS_IMPORT_RUNTIME_STARTED,
	NATIVE_IOS_IMPORT_RELAUNCH_REQUIRED,
};

typedef enum NativeIOSImportValidationResult (*NativeIOSImportValidateCallback)(const char *stagingBasePath, char *detail,
	                                                                            size_t detailSize, void *userdata);
typedef enum NativeIOSImportCompletionResult (*NativeIOSImportCompletionCallback)(void *userdata);

int NativeIOSImport_RecoverStaleStages(const char *importBaseDir);
int NativeIOSImport_Begin(const char *importBaseDir, enum NativeIOSImportPurpose purpose,
	                      NativeIOSImportValidateCallback validateCallback,
	                      NativeIOSImportCompletionCallback completionCallback, void *userdata);

#endif

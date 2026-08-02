#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <string.h>

#include "platform/native_macos_import.h"

static NSString *const s_discPathKey = @"CTRPadRetailDiscImagePath";

static int NativeMacOSImport_CopyPath(NSString *path, char *dst, size_t dstSize)
{
	const char *fileSystemPath;
	if ((path.length == 0) || (dst == NULL) || (dstSize == 0))
	{
		return 0;
	}
	fileSystemPath = path.fileSystemRepresentation;
	if ((fileSystemPath == NULL) || (strlen(fileSystemPath) >= dstSize))
	{
		return 0;
	}
	memcpy(dst, fileSystemPath, strlen(fileSystemPath) + 1u);
	return 1;
}

int NativeMacOSImport_GetRememberedDiscPath(char *dst, size_t dstSize)
{
	@autoreleasepool
	{
		NSString *path = [NSUserDefaults.standardUserDefaults stringForKey:s_discPathKey];
		BOOL directory = NO;
		if ((path.length == 0) || ![NSFileManager.defaultManager fileExistsAtPath:path isDirectory:&directory] || directory)
		{
			return 0;
		}
		return NativeMacOSImport_CopyPath(path, dst, dstSize);
	}
}

int NativeMacOSImport_ChooseDiscPath(char *dst, size_t dstSize)
{
	@autoreleasepool
	{
		NSOpenPanel *panel = [NSOpenPanel openPanel];
		panel.title = @"Choose your Crash Team Racing disc image";
		panel.message = @"Select your own NTSC-U single-track raw MODE2/2352 BIN. CTRPad remembers its location and does not place it inside the app.";
		panel.prompt = @"Choose Disc Image";
		panel.canChooseDirectories = NO;
		panel.canChooseFiles = YES;
		panel.allowsMultipleSelection = NO;
		panel.resolvesAliases = YES;
		UTType *binType = [UTType typeWithFilenameExtension:@"bin"];
		if (binType != nil)
		{
			panel.allowedContentTypes = @[ binType ];
		}
		if ([panel runModal] != NSModalResponseOK)
		{
			return 0;
		}
		return NativeMacOSImport_CopyPath(panel.URL.path, dst, dstSize) ? 1 : -1;
	}
}

void NativeMacOSImport_RememberDiscPath(const char *path)
{
	@autoreleasepool
	{
		if ((path == NULL) || (path[0] == '\0'))
		{
			return;
		}
		NSString *value = [NSFileManager.defaultManager stringWithFileSystemRepresentation:path length:strlen(path)];
		[NSUserDefaults.standardUserDefaults setObject:value forKey:s_discPathKey];
	}
}

void NativeMacOSImport_ForgetDiscPath(void)
{
	@autoreleasepool
	{
		[NSUserDefaults.standardUserDefaults removeObjectForKey:s_discPathKey];
	}
}

void NativeMacOSImport_ShowInvalidDiscAlert(void)
{
	@autoreleasepool
	{
		NSAlert *alert = [[NSAlert alloc] init];
		alert.alertStyle = NSAlertStyleWarning;
		alert.messageText = @"That disc image is not compatible with CTRPad";
		alert.informativeText = @"Choose the NTSC-U single-track raw MODE2/2352 BIN data track. CUE files, compressed archives, cooked ISOs, other regions, and incomplete images are not accepted.";
		[alert addButtonWithTitle:@"Choose Another File"];
		[alert runModal];
	}
}

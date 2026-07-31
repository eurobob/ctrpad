#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include "platform/native_ios_import.h"

@class CTRPadImportCoordinator;

@interface CTRPadImportViewController : UIViewController
@property(nonatomic, weak) CTRPadImportCoordinator *coordinator;
@property(nonatomic, strong) UILabel *statusLabel;
@property(nonatomic, strong) UIButton *chooseButton;
@property(nonatomic, strong) UIActivityIndicatorView *activityIndicator;
- (void)setBusy:(BOOL)busy status:(NSString *)status;
@end

@interface CTRPadImportCoordinator : NSObject <UIDocumentPickerDelegate>
@property(nonatomic, strong) UIWindow *window;
@property(nonatomic, strong) CTRPadImportViewController *viewController;
@property(nonatomic, copy) NSString *importBasePath;
@property(nonatomic, assign) NativeIOSImportValidateCallback validateCallback;
@property(nonatomic, assign) NativeIOSImportCompletionCallback completionCallback;
@property(nonatomic, assign) void *callbackUserdata;
- (BOOL)start;
- (NSInteger)removeStaleStagingDirectories;
- (void)presentPicker;
- (void)beginImportFromURL:(NSURL *)sourceURL;
- (void)finishWithError:(NSString *)message;
@end

static CTRPadImportCoordinator *s_importCoordinator;
static NSString *const s_importStagingPrefix = @".ctrpad-import-";

@implementation CTRPadImportViewController

- (void)viewDidLoad
{
	[super viewDidLoad];

	self.view.backgroundColor = [UIColor colorWithRed:0.025 green:0.047 blue:0.090 alpha:1.0];

	UILabel *titleLabel = [[UILabel alloc] init];
	titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
	titleLabel.text = @"CTRPad";
	titleLabel.textColor = UIColor.whiteColor;
	titleLabel.font = [UIFont systemFontOfSize:42.0 weight:UIFontWeightBlack];
	titleLabel.textAlignment = NSTextAlignmentCenter;

	UILabel *headingLabel = [[UILabel alloc] init];
	headingLabel.translatesAutoresizingMaskIntoConstraints = NO;
	headingLabel.text = @"Your retail disc image is required";
	headingLabel.textColor = [UIColor colorWithRed:0.98 green:0.73 blue:0.17 alpha:1.0];
	headingLabel.font = [UIFont systemFontOfSize:24.0 weight:UIFontWeightBold];
	headingLabel.textAlignment = NSTextAlignmentCenter;
	headingLabel.numberOfLines = 0;

	UILabel *bodyLabel = [[UILabel alloc] init];
	bodyLabel.translatesAutoresizingMaskIntoConstraints = NO;
	bodyLabel.text = @"Choose your own NTSC-U Crash Team Racing BIN in raw MODE2/2352 format. CTRPad copies it into this app's Documents folder; the original file is left unchanged.";
	bodyLabel.textColor = [UIColor colorWithWhite:0.86 alpha:1.0];
	bodyLabel.font = [UIFont systemFontOfSize:17.0 weight:UIFontWeightRegular];
	bodyLabel.textAlignment = NSTextAlignmentCenter;
	bodyLabel.numberOfLines = 0;

	UIButtonConfiguration *buttonConfiguration = [UIButtonConfiguration filledButtonConfiguration];
	buttonConfiguration.title = @"Choose CTR disc image";
	buttonConfiguration.baseBackgroundColor = [UIColor colorWithRed:0.13 green:0.42 blue:0.84 alpha:1.0];
	buttonConfiguration.baseForegroundColor = UIColor.whiteColor;
	buttonConfiguration.cornerStyle = UIButtonConfigurationCornerStyleLarge;
	buttonConfiguration.contentInsets = NSDirectionalEdgeInsetsMake(14.0, 22.0, 14.0, 22.0);
	self.chooseButton = [UIButton buttonWithConfiguration:buttonConfiguration primaryAction:nil];
	self.chooseButton.translatesAutoresizingMaskIntoConstraints = NO;
	self.chooseButton.accessibilityIdentifier = @"ctrpad.import.choose";
	[self.chooseButton addTarget:self.coordinator action:@selector(presentPicker) forControlEvents:UIControlEventTouchUpInside];

	self.activityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleMedium];
	self.activityIndicator.translatesAutoresizingMaskIntoConstraints = NO;
	self.activityIndicator.color = UIColor.whiteColor;
	self.activityIndicator.hidesWhenStopped = YES;

	self.statusLabel = [[UILabel alloc] init];
	self.statusLabel.translatesAutoresizingMaskIntoConstraints = NO;
	self.statusLabel.text = @"No game data is bundled with CTRPad.";
	self.statusLabel.textColor = [UIColor colorWithWhite:0.68 alpha:1.0];
	self.statusLabel.font = [UIFont systemFontOfSize:15.0 weight:UIFontWeightMedium];
	self.statusLabel.textAlignment = NSTextAlignmentCenter;
	self.statusLabel.numberOfLines = 0;
	self.statusLabel.accessibilityIdentifier = @"ctrpad.import.status";

	UIStackView *statusStack = [[UIStackView alloc] initWithArrangedSubviews:@[ self.activityIndicator, self.statusLabel ]];
	statusStack.translatesAutoresizingMaskIntoConstraints = NO;
	statusStack.axis = UILayoutConstraintAxisHorizontal;
	statusStack.alignment = UIStackViewAlignmentCenter;
	statusStack.spacing = 10.0;

	UIStackView *contentStack = [[UIStackView alloc] initWithArrangedSubviews:@[ titleLabel, headingLabel, bodyLabel, self.chooseButton, statusStack ]];
	contentStack.translatesAutoresizingMaskIntoConstraints = NO;
	contentStack.axis = UILayoutConstraintAxisVertical;
	contentStack.alignment = UIStackViewAlignmentCenter;
	contentStack.spacing = 22.0;

	[self.view addSubview:contentStack];
	[NSLayoutConstraint activateConstraints:@[
		[contentStack.centerXAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.centerXAnchor],
		[contentStack.centerYAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.centerYAnchor],
		[contentStack.leadingAnchor constraintGreaterThanOrEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor constant:32.0],
		[contentStack.trailingAnchor constraintLessThanOrEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor constant:-32.0],
		[contentStack.widthAnchor constraintLessThanOrEqualToConstant:680.0],
		[bodyLabel.widthAnchor constraintEqualToAnchor:contentStack.widthAnchor],
		[self.statusLabel.widthAnchor constraintLessThanOrEqualToConstant:580.0],
	]];
}

- (void)setBusy:(BOOL)busy status:(NSString *)status
{
	self.chooseButton.enabled = !busy;
	self.statusLabel.text = status;
	if (busy)
	{
		[self.activityIndicator startAnimating];
	}
	else
	{
		[self.activityIndicator stopAnimating];
	}
}

- (BOOL)prefersStatusBarHidden
{
	return YES;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
	return UIInterfaceOrientationMaskLandscape;
}

@end

@implementation CTRPadImportCoordinator

- (NSInteger)removeStaleStagingDirectories
{
	NSFileManager *fileManager = NSFileManager.defaultManager;
	NSError *listingError = nil;
	NSArray<NSString *> *entries = [fileManager contentsOfDirectoryAtPath:self.importBasePath error:&listingError];
	NSInteger removedCount = 0;

	if (entries == nil)
	{
		if ((listingError != nil) &&
		    !([listingError.domain isEqualToString:NSCocoaErrorDomain] && (listingError.code == NSFileNoSuchFileError)))
		{
			NSLog(@"[CTR Import] Could not inspect interrupted imports: %@", listingError.localizedDescription);
		}
		return 0;
	}

	for (NSString *entry in entries)
	{
		if (![entry hasPrefix:s_importStagingPrefix] || (entry.length == s_importStagingPrefix.length))
		{
			continue;
		}

		NSString *candidatePath = [self.importBasePath stringByAppendingPathComponent:entry];
		BOOL isDirectory = NO;
		if (![fileManager fileExistsAtPath:candidatePath isDirectory:&isDirectory] || !isDirectory)
		{
			continue;
		}

		NSError *removeError = nil;
		if ([fileManager removeItemAtPath:candidatePath error:&removeError])
		{
			removedCount++;
		}
		else
		{
			NSLog(@"[CTR Import] Could not remove interrupted import %@: %@", entry, removeError.localizedDescription);
		}
	}

	return removedCount;
}

- (UIWindowScene *)activeWindowScene
{
	for (UIScene *scene in UIApplication.sharedApplication.connectedScenes)
	{
		if ([scene isKindOfClass:UIWindowScene.class] && scene.activationState != UISceneActivationStateUnattached)
		{
			return (UIWindowScene *)scene;
		}
	}
	return nil;
}

- (BOOL)start
{
	UIWindowScene *windowScene = [self activeWindowScene];
	NSInteger recoveredImportCount;
	if (windowScene == nil)
	{
		return NO;
	}
	recoveredImportCount = [self removeStaleStagingDirectories];

	self.viewController = [[CTRPadImportViewController alloc] init];
	self.viewController.coordinator = self;
	self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
	self.window.frame = windowScene.coordinateSpace.bounds;
	self.window.windowLevel = UIWindowLevelNormal + 2.0;
	self.window.rootViewController = self.viewController;
	[self.window makeKeyAndVisible];
	if (recoveredImportCount != 0)
	{
		NSString *status;
		if (recoveredImportCount == 1)
		{
			status = @"Recovered an interrupted import. No partial image was installed; choose your NTSC-U raw BIN to retry.";
		}
		else
		{
			status = [NSString stringWithFormat:@"Recovered %ld interrupted imports. No partial image was installed; choose your NTSC-U raw BIN to retry.",
			                                             (long)recoveredImportCount];
		}
		[self.viewController setBusy:NO status:status];
	}
	return YES;
}

- (void)presentPicker
{
	if (self.viewController.presentedViewController != nil)
	{
		return;
	}

	UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[ UTTypeData ] asCopy:YES];
	picker.delegate = self;
	picker.allowsMultipleSelection = NO;
	picker.shouldShowFileExtensions = YES;
	picker.modalPresentationStyle = UIModalPresentationFormSheet;
	[self.viewController presentViewController:picker animated:YES completion:nil];
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller
{
	(void)controller;
	[self.viewController setBusy:NO status:@"No file selected. Choose the NTSC-U raw BIN when you are ready."];
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls
{
	NSURL *sourceURL = urls.firstObject;
	if (sourceURL == nil)
	{
		[self.viewController setBusy:NO status:@"Files did not return a readable selection. Please try again."];
		return;
	}

	[controller dismissViewControllerAnimated:YES
	                              completion:^{
	                                [self beginImportFromURL:sourceURL];
	                              }];
}

- (NSString *)messageForValidationResult:(enum NativeIOSImportValidationResult)result detail:(NSString *)detail
{
	switch (result)
	{
	case NATIVE_IOS_IMPORT_INVALID_FORMAT:
		return @"That file is not a readable raw MODE2/2352 disc image. Select the BIN data track, not a CUE or compressed archive.";
	case NATIVE_IOS_IMPORT_WRONG_REGION:
		return detail.length != 0 ? [NSString stringWithFormat:@"Wrong disc region (%@). CTRPad currently requires NTSC-U SCUS-94426.", detail]
		                          : @"Wrong disc region. CTRPad currently requires NTSC-U SCUS-94426.";
	case NATIVE_IOS_IMPORT_INCOMPLETE:
		return @"The disc image opened, but required CTR files were missing or unreadable. The existing import was not replaced.";
	case NATIVE_IOS_IMPORT_VALID:
		return @"Disc image verified.";
	}
	return @"The disc image could not be verified.";
}

- (void)beginImportFromURL:(NSURL *)sourceURL
{
	[self.viewController setBusy:YES status:@"Copying the disc image into CTRPad… Keep the app open."];

	NSString *importBasePath = self.importBasePath;
	NativeIOSImportValidateCallback validateCallback = self.validateCallback;
	NativeIOSImportCompletionCallback completionCallback = self.completionCallback;
	void *callbackUserdata = self.callbackUserdata;
	__weak CTRPadImportCoordinator *weakSelf = self;

	dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
		@autoreleasepool
		{
			CTRPadImportCoordinator *strongSelf = weakSelf;
			if (strongSelf == nil)
			{
				return;
			}

			NSFileManager *fileManager = NSFileManager.defaultManager;
				NSString *stagingBasePath = [importBasePath stringByAppendingPathComponent:[s_importStagingPrefix stringByAppendingString:NSUUID.UUID.UUIDString]];
			NSString *stagingAssetPath = [[stagingBasePath stringByAppendingPathComponent:@"assets"] stringByAppendingPathComponent:@"ctr-u.bin"];
			NSString *destinationAssetPath = [[importBasePath stringByAppendingPathComponent:@"assets"] stringByAppendingPathComponent:@"ctr-u.bin"];
			NSURL *stagingAssetURL = [NSURL fileURLWithPath:stagingAssetPath];
			NSURL *destinationAssetURL = [NSURL fileURLWithPath:destinationAssetPath];
			NSError *error = nil;
			BOOL scopedAccess = [sourceURL startAccessingSecurityScopedResource];

			if (![fileManager createDirectoryAtPath:stagingAssetPath.stringByDeletingLastPathComponent
			                          withIntermediateDirectories:YES
			                                           attributes:nil
			                                                error:&error])
			{
				if (scopedAccess)
				{
					[sourceURL stopAccessingSecurityScopedResource];
				}
				[strongSelf finishWithError:[NSString stringWithFormat:@"Could not prepare the import folder: %@", error.localizedDescription]];
				return;
			}

			NSFileCoordinator *fileCoordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
			__block NSError *copyError = nil;
			NSError *coordinationError = nil;
			[fileCoordinator coordinateReadingItemAtURL:sourceURL
			                                     options:NSFileCoordinatorReadingWithoutChanges
			                                       error:&coordinationError
			                                  byAccessor:^(NSURL *coordinatedURL) {
			                                    [fileManager copyItemAtURL:coordinatedURL toURL:stagingAssetURL error:&copyError];
			                                  }];

			if (scopedAccess)
			{
				[sourceURL stopAccessingSecurityScopedResource];
			}

			if ((coordinationError != nil) || (copyError != nil))
			{
				[fileManager removeItemAtPath:stagingBasePath error:nil];
				NSError *reportedError = copyError ?: coordinationError;
				[strongSelf finishWithError:[NSString stringWithFormat:@"Could not copy the selected file: %@", reportedError.localizedDescription]];
				return;
			}

			dispatch_async(dispatch_get_main_queue(), ^{
			  [strongSelf.viewController setBusy:YES status:@"Checking disc format, region, and required game data…"];
			});

			char detail[128] = {0};
			enum NativeIOSImportValidationResult validationResult = validateCallback(stagingBasePath.fileSystemRepresentation, detail, sizeof(detail), callbackUserdata);
			if (validationResult != NATIVE_IOS_IMPORT_VALID)
			{
				[fileManager removeItemAtPath:stagingBasePath error:nil];
				NSString *detailString = detail[0] != '\0' ? [NSString stringWithUTF8String:detail] : @"";
				NSString *message = [strongSelf messageForValidationResult:validationResult detail:detailString];
				[strongSelf finishWithError:message];
				return;
			}

			error = nil;
			[fileManager createDirectoryAtPath:destinationAssetPath.stringByDeletingLastPathComponent
			      withIntermediateDirectories:YES
			                       attributes:nil
			                            error:&error];
			if (error == nil)
			{
				if ([fileManager fileExistsAtPath:destinationAssetPath])
				{
					[fileManager replaceItemAtURL:destinationAssetURL
					                  withItemAtURL:stagingAssetURL
					                 backupItemName:nil
					                        options:NSFileManagerItemReplacementUsingNewMetadataOnly
					               resultingItemURL:nil
					                          error:&error];
				}
				else
				{
					[fileManager moveItemAtURL:stagingAssetURL toURL:destinationAssetURL error:&error];
				}
			}
			[fileManager removeItemAtPath:stagingBasePath error:nil];

			if (error != nil)
			{
				[strongSelf finishWithError:[NSString stringWithFormat:@"The image verified, but CTRPad could not install it: %@", error.localizedDescription]];
				return;
			}

			dispatch_async(dispatch_get_main_queue(), ^{
			  CTRPadImportCoordinator *mainSelf = weakSelf;
			  if (mainSelf == nil)
			  {
				  return;
			  }
			  [mainSelf.viewController setBusy:YES status:@"Disc verified. Starting Crash Team Racing…"];
			  if (!completionCallback(callbackUserdata))
			  {
				  [mainSelf.viewController setBusy:NO status:@"The image installed, but the game could not start. Close and reopen CTRPad to retry."];
				  return;
			  }
			  mainSelf.window.hidden = YES;
			  mainSelf.window = nil;
			  mainSelf.viewController = nil;
			  s_importCoordinator = nil;
			});
		}
	});
}

- (void)finishWithError:(NSString *)message
{
	dispatch_async(dispatch_get_main_queue(), ^{
	  [self.viewController setBusy:NO status:message];
	});
}

@end

int NativeIOSImport_RecoverStaleStages(const char *importBaseDir)
{
	if ((importBaseDir == NULL) || (importBaseDir[0] == '\0'))
	{
		return -1;
	}

	NSString *basePath = [NSString stringWithUTF8String:importBaseDir];
	if (basePath.length == 0)
	{
		return -1;
	}

	CTRPadImportCoordinator *coordinator = [[CTRPadImportCoordinator alloc] init];
	coordinator.importBasePath = basePath;
	return (int)[coordinator removeStaleStagingDirectories];
}

int NativeIOSImport_Begin(const char *importBaseDir, NativeIOSImportValidateCallback validateCallback,
	                      NativeIOSImportCompletionCallback completionCallback, void *userdata)
{
	if ((importBaseDir == NULL) || (importBaseDir[0] == '\0') || (validateCallback == NULL) || (completionCallback == NULL) ||
	    !NSThread.isMainThread || (s_importCoordinator != nil))
	{
		return 0;
	}

	NSString *basePath = [NSString stringWithUTF8String:importBaseDir];
	if (basePath.length == 0)
	{
		return 0;
	}

	CTRPadImportCoordinator *coordinator = [[CTRPadImportCoordinator alloc] init];
	coordinator.importBasePath = basePath;
	coordinator.validateCallback = validateCallback;
	coordinator.completionCallback = completionCallback;
	coordinator.callbackUserdata = userdata;
	if (![coordinator start])
	{
		return 0;
	}

	s_importCoordinator = coordinator;
	return 1;
}

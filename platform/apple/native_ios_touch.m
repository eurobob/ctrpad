#import <UIKit/UIKit.h>

#include <math.h>

#include "platform/native_input.h"
#include "platform/native_ios_touch.h"

typedef NS_ENUM(NSInteger, CTRPadTouchHandedness)
{
	CTRPadTouchHandednessSteerLeft = 0,
	CTRPadTouchHandednessSteerRight,
};

typedef NS_ENUM(NSInteger, CTRPadTouchSize)
{
	CTRPadTouchSizeSmall = 0,
	CTRPadTouchSizeStandard,
	CTRPadTouchSizeLarge,
};

typedef NS_ENUM(NSInteger, CTRPadTouchOpacity)
{
	CTRPadTouchOpacityLow = 0,
	CTRPadTouchOpacityStandard,
	CTRPadTouchOpacityHigh,
};

static NSString *const s_touchHandednessKey = @"CTRPadTouchHandedness";
static NSString *const s_touchSizeKey = @"CTRPadTouchSize";
static NSString *const s_touchOpacityKey = @"CTRPadTouchOpacity";

@class CTRPadTouchOverlayViewController;

@interface CTRPadTouchPassthroughView : UIView
@end

@interface CTRPadTouchStickView : UIView
@property(nonatomic, strong) UIView *knob;
@property(nonatomic, strong) UILabel *label;
@property(nonatomic, assign) BOOL trackingTouch;
@property(nonatomic, assign) unsigned int directionMask;
- (void)applyControlOpacity:(CGFloat)opacity;
@end

@interface CTRPadTouchOverlayViewController : UIViewController
@property(nonatomic, assign) CTRPadTouchHandedness handedness;
@property(nonatomic, assign) CGFloat controlScale;
@property(nonatomic, assign) CGFloat controlOpacity;
- (void)applyPreferencesAndRebuildControls;
@end

@interface CTRPadTouchSettingsViewController : UIViewController
@property(nonatomic, weak) CTRPadTouchOverlayViewController *overlayController;
@property(nonatomic, strong) UISegmentedControl *handednessControl;
@property(nonatomic, strong) UISegmentedControl *sizeControl;
@property(nonatomic, strong) UISegmentedControl *opacityControl;
@end

static CTRPadTouchOverlayViewController *s_touchOverlayController;
static NativeIOSTouchDiscReselectionCallback s_discReselectionCallback;
static void *s_discReselectionUserdata;

static NSInteger CTRPadTouch_ReadPreference(NSString *key, NSInteger defaultValue, NSInteger maximumValue)
{
	id value = [NSUserDefaults.standardUserDefaults objectForKey:key];
	if (![value isKindOfClass:NSNumber.class])
	{
		return defaultValue;
	}

	NSInteger choice = [value integerValue];
	return ((choice >= 0) && (choice <= maximumValue)) ? choice : defaultValue;
}

static CGFloat CTRPadTouch_ScaleForChoice(CTRPadTouchSize choice)
{
	switch (choice)
	{
	case CTRPadTouchSizeSmall:
		return 0.88;
	case CTRPadTouchSizeLarge:
		return 1.12;
	case CTRPadTouchSizeStandard:
		return 1.0;
	}
	return 1.0;
}

static CGFloat CTRPadTouch_OpacityForChoice(CTRPadTouchOpacity choice)
{
	switch (choice)
	{
	case CTRPadTouchOpacityLow:
		return 0.36;
	case CTRPadTouchOpacityHigh:
		return 0.80;
	case CTRPadTouchOpacityStandard:
		return 0.58;
	}
	return 0.58;
}

@implementation CTRPadTouchPassthroughView

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event
{
	UIView *result = [super hitTest:point withEvent:event];
	return result == self ? nil : result;
}

@end

@implementation CTRPadTouchStickView

- (instancetype)init
{
	self = [super initWithFrame:CGRectZero];
	if (self != nil)
	{
		self.translatesAutoresizingMaskIntoConstraints = NO;
		self.multipleTouchEnabled = NO;
		self.accessibilityIdentifier = @"ctrpad.touch.stick";
		self.accessibilityLabel = @"Steering stick";
		self.isAccessibilityElement = YES;
		self.backgroundColor = [UIColor colorWithWhite:0.04 alpha:0.42];
		self.layer.borderColor = [UIColor colorWithWhite:1.0 alpha:0.42].CGColor;
		self.layer.borderWidth = 2.0;

		self.knob = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 70, 70)];
		self.knob.userInteractionEnabled = NO;
		self.knob.backgroundColor = [UIColor colorWithRed:0.16 green:0.48 blue:0.95 alpha:0.76];
		self.knob.layer.borderColor = [UIColor colorWithWhite:1.0 alpha:0.72].CGColor;
		self.knob.layer.borderWidth = 2.0;
		[self addSubview:self.knob];

		self.label = [[UILabel alloc] initWithFrame:CGRectZero];
		self.label.translatesAutoresizingMaskIntoConstraints = NO;
		self.label.text = @"STEER";
		self.label.textColor = [UIColor colorWithWhite:1.0 alpha:0.74];
		self.label.font = [UIFont systemFontOfSize:12.0 weight:UIFontWeightBold];
		self.label.userInteractionEnabled = NO;
		[self addSubview:self.label];
		[NSLayoutConstraint activateConstraints:@[
			[self.label.centerXAnchor constraintEqualToAnchor:self.centerXAnchor],
			[self.label.bottomAnchor constraintEqualToAnchor:self.bottomAnchor constant:-12.0],
		]];
	}
	return self;
}

- (void)applyControlOpacity:(CGFloat)opacity
{
	self.backgroundColor = [UIColor colorWithWhite:0.04 alpha:opacity * 0.72];
	self.layer.borderColor = [UIColor colorWithWhite:1.0 alpha:MAX(0.42, opacity * 0.82)].CGColor;
	self.knob.backgroundColor = [UIColor colorWithRed:0.16 green:0.48 blue:0.95 alpha:MIN(1.0, opacity + 0.18)];
}

- (void)layoutSubviews
{
	[super layoutSubviews];
	self.layer.cornerRadius = MIN(self.bounds.size.width, self.bounds.size.height) * 0.5;
	self.knob.layer.cornerRadius = self.knob.bounds.size.width * 0.5;
	if (!self.trackingTouch)
	{
		self.knob.center = CGPointMake(CGRectGetMidX(self.bounds), CGRectGetMidY(self.bounds));
	}
}

- (void)publishPoint:(CGPoint)point
{
	CGPoint center = CGPointMake(CGRectGetMidX(self.bounds), CGRectGetMidY(self.bounds));
	CGFloat dx = point.x - center.x;
	CGFloat dy = point.y - center.y;
	CGFloat length = hypot(dx, dy);
	CGFloat radius = MAX(1.0, MIN(self.bounds.size.width, self.bounds.size.height) * 0.5 - self.knob.bounds.size.width * 0.5 - 4.0);
	unsigned int directionMask = 0;
	if (length > radius)
	{
		dx = dx * radius / length;
		dy = dy * radius / length;
	}
	self.knob.center = CGPointMake(center.x + dx, center.y + dy);
	Platform_InputTouchLeftStick((int)lrint(dx * 32767.0 / radius), (int)lrint(dy * 32767.0 / radius), 1);

	// Preserve the full analog range for racing. The outer ring additionally
	// publishes retail D-pad edges so brief menu gestures are never dependent
	// on the game's optional analog-to-button setting.
	CGFloat directionThreshold = radius * 0.68;
	if (dx <= -directionThreshold)
		directionMask |= PLATFORM_INPUT_TOUCH_LEFT;
	if (dx >= directionThreshold)
		directionMask |= PLATFORM_INPUT_TOUCH_RIGHT;
	if (dy <= -directionThreshold)
		directionMask |= PLATFORM_INPUT_TOUCH_UP;
	if (dy >= directionThreshold)
		directionMask |= PLATFORM_INPUT_TOUCH_DOWN;

	unsigned int releasedDirections = self.directionMask & ~directionMask;
	unsigned int pressedDirections = directionMask & ~self.directionMask;
	if (releasedDirections != 0)
		Platform_InputTouchButton(releasedDirections, 0);
	if (pressedDirections != 0)
		Platform_InputTouchButton(pressedDirections, 1);
	self.directionMask = directionMask;
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	(void)event;
	UITouch *touch = touches.anyObject;
	if (touch == nil)
	{
		return;
	}
	self.trackingTouch = YES;
	[self publishPoint:[touch locationInView:self]];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	(void)event;
	UITouch *touch = touches.anyObject;
	if ((touch != nil) && self.trackingTouch)
	{
		[self publishPoint:[touch locationInView:self]];
	}
}

- (void)releaseTouch
{
	self.trackingTouch = NO;
	self.knob.center = CGPointMake(CGRectGetMidX(self.bounds), CGRectGetMidY(self.bounds));
	if (self.directionMask != 0)
	{
		Platform_InputTouchButton(self.directionMask, 0);
		self.directionMask = 0;
	}
	Platform_InputTouchLeftStick(0, 0, 0);
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	(void)touches;
	(void)event;
	[self releaseTouch];
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	(void)touches;
	(void)event;
	[self releaseTouch];
}

@end

@implementation CTRPadTouchSettingsViewController

- (UILabel *)sectionLabelWithText:(NSString *)text
{
	UILabel *label = [[UILabel alloc] init];
	label.text = text;
	label.textColor = [UIColor colorWithWhite:0.86 alpha:1.0];
	label.font = [UIFont systemFontOfSize:15.0 weight:UIFontWeightSemibold];
	return label;
}

- (void)viewDidLoad
{
	[super viewDidLoad];
	self.view.backgroundColor = [UIColor colorWithRed:0.025 green:0.035 blue:0.075 alpha:1.0];
	self.preferredContentSize = CGSizeMake(520.0, 560.0);

	UILabel *titleLabel = [[UILabel alloc] init];
	titleLabel.text = @"Controls";
	titleLabel.textColor = UIColor.whiteColor;
	titleLabel.font = [UIFont systemFontOfSize:28.0 weight:UIFontWeightBold];
	titleLabel.textAlignment = NSTextAlignmentCenter;

	UILabel *bodyLabel = [[UILabel alloc] init];
	bodyLabel.text = @"Touch changes apply immediately and stay on this device. Every interactive target remains at least 44 points.";
	bodyLabel.textColor = [UIColor colorWithWhite:0.72 alpha:1.0];
	bodyLabel.font = [UIFont systemFontOfSize:15.0 weight:UIFontWeightRegular];
	bodyLabel.textAlignment = NSTextAlignmentCenter;
	bodyLabel.numberOfLines = 0;

	self.handednessControl = [[UISegmentedControl alloc] initWithItems:@[ @"Steer left", @"Steer right" ]];
	self.handednessControl.selectedSegmentIndex = CTRPadTouch_ReadPreference(s_touchHandednessKey,
	                                                                        CTRPadTouchHandednessSteerLeft,
	                                                                        CTRPadTouchHandednessSteerRight);
	self.handednessControl.accessibilityIdentifier = @"ctrpad.touch.settings.handedness";
	[self.handednessControl addTarget:self action:@selector(preferencesChanged) forControlEvents:UIControlEventValueChanged];

	self.sizeControl = [[UISegmentedControl alloc] initWithItems:@[ @"Small", @"Standard", @"Large" ]];
	self.sizeControl.selectedSegmentIndex = CTRPadTouch_ReadPreference(s_touchSizeKey, CTRPadTouchSizeStandard, CTRPadTouchSizeLarge);
	self.sizeControl.accessibilityIdentifier = @"ctrpad.touch.settings.size";
	[self.sizeControl addTarget:self action:@selector(preferencesChanged) forControlEvents:UIControlEventValueChanged];

	self.opacityControl = [[UISegmentedControl alloc] initWithItems:@[ @"Low", @"Standard", @"High" ]];
	self.opacityControl.selectedSegmentIndex = CTRPadTouch_ReadPreference(s_touchOpacityKey,
	                                                                     CTRPadTouchOpacityStandard,
	                                                                     CTRPadTouchOpacityHigh);
	self.opacityControl.accessibilityIdentifier = @"ctrpad.touch.settings.opacity";
	[self.opacityControl addTarget:self action:@selector(preferencesChanged) forControlEvents:UIControlEventValueChanged];

	UILabel *keyboardLabel = [self sectionLabelWithText:@"Keyboard test layout"];
	UILabel *keyboardMap = [[UILabel alloc] init];
	keyboardMap.text = @"Steer / menus   W A S D\n"
	                   @"View Brake Gas Item   I J K L\n"
	                   @"Drift / boost   Q / E\n"
	                   @"Pause / Select   P / Tab";
	keyboardMap.textColor = [UIColor colorWithWhite:0.76 alpha:1.0];
	keyboardMap.font = [UIFont monospacedSystemFontOfSize:14.0 weight:UIFontWeightRegular];
	keyboardMap.numberOfLines = 0;
	keyboardMap.accessibilityLabel = @"Keyboard controls. W A S D steer and navigate menus. I J K L are view, brake, gas, and item. Q and E drift and boost. P pauses. Tab selects.";

	UIButtonConfiguration *resetConfiguration = [UIButtonConfiguration tintedButtonConfiguration];
	resetConfiguration.title = @"Reset defaults";
	resetConfiguration.baseForegroundColor = [UIColor colorWithRed:1.0 green:0.66 blue:0.19 alpha:1.0];
	UIButton *resetButton = [UIButton buttonWithConfiguration:resetConfiguration primaryAction:nil];
	resetButton.accessibilityIdentifier = @"ctrpad.touch.settings.reset";
	[resetButton addTarget:self action:@selector(resetDefaults) forControlEvents:UIControlEventTouchUpInside];

	UIButtonConfiguration *doneConfiguration = [UIButtonConfiguration filledButtonConfiguration];
	doneConfiguration.title = @"Done";
	doneConfiguration.baseBackgroundColor = [UIColor colorWithRed:0.13 green:0.42 blue:0.84 alpha:1.0];
	UIButton *doneButton = [UIButton buttonWithConfiguration:doneConfiguration primaryAction:nil];
	doneButton.accessibilityIdentifier = @"ctrpad.touch.settings.done";
	[doneButton addTarget:self action:@selector(done) forControlEvents:UIControlEventTouchUpInside];

	UIStackView *stack = [[UIStackView alloc] initWithArrangedSubviews:@[
		titleLabel,
		bodyLabel,
		[self sectionLabelWithText:@"Handedness"],
		self.handednessControl,
		[self sectionLabelWithText:@"Control size"],
		self.sizeControl,
		[self sectionLabelWithText:@"Control opacity"],
		self.opacityControl,
		keyboardLabel,
		keyboardMap,
		resetButton,
		doneButton,
	]];
	stack.translatesAutoresizingMaskIntoConstraints = NO;
	stack.axis = UILayoutConstraintAxisVertical;
	stack.alignment = UIStackViewAlignmentFill;
	stack.spacing = 12.0;
	[stack setCustomSpacing:22.0 afterView:bodyLabel];
	[stack setCustomSpacing:22.0 afterView:self.opacityControl];
	[stack setCustomSpacing:20.0 afterView:keyboardMap];
	UIScrollView *scrollView = [[UIScrollView alloc] init];
	scrollView.translatesAutoresizingMaskIntoConstraints = NO;
	scrollView.alwaysBounceVertical = NO;
	[self.view addSubview:scrollView];
	[scrollView addSubview:stack];

	UILayoutGuide *safe = self.view.safeAreaLayoutGuide;
	[NSLayoutConstraint activateConstraints:@[
		[scrollView.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor],
		[scrollView.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor],
		[scrollView.topAnchor constraintEqualToAnchor:safe.topAnchor],
		[scrollView.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor],
		[stack.leadingAnchor constraintGreaterThanOrEqualToAnchor:scrollView.frameLayoutGuide.leadingAnchor constant:24.0],
		[stack.trailingAnchor constraintLessThanOrEqualToAnchor:scrollView.frameLayoutGuide.trailingAnchor constant:-24.0],
		[stack.topAnchor constraintEqualToAnchor:scrollView.contentLayoutGuide.topAnchor constant:24.0],
		[stack.bottomAnchor constraintEqualToAnchor:scrollView.contentLayoutGuide.bottomAnchor constant:-24.0],
		[stack.centerXAnchor constraintEqualToAnchor:scrollView.frameLayoutGuide.centerXAnchor],
		[stack.widthAnchor constraintLessThanOrEqualToConstant:460.0],
		[self.handednessControl.heightAnchor constraintGreaterThanOrEqualToConstant:44.0],
		[self.sizeControl.heightAnchor constraintGreaterThanOrEqualToConstant:44.0],
		[self.opacityControl.heightAnchor constraintGreaterThanOrEqualToConstant:44.0],
		[resetButton.heightAnchor constraintGreaterThanOrEqualToConstant:44.0],
		[doneButton.heightAnchor constraintGreaterThanOrEqualToConstant:50.0],
	]];
}

- (void)preferencesChanged
{
	NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
	[defaults setInteger:self.handednessControl.selectedSegmentIndex forKey:s_touchHandednessKey];
	[defaults setInteger:self.sizeControl.selectedSegmentIndex forKey:s_touchSizeKey];
	[defaults setInteger:self.opacityControl.selectedSegmentIndex forKey:s_touchOpacityKey];
	[self.overlayController applyPreferencesAndRebuildControls];
}

- (void)resetDefaults
{
	NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
	[defaults removeObjectForKey:s_touchHandednessKey];
	[defaults removeObjectForKey:s_touchSizeKey];
	[defaults removeObjectForKey:s_touchOpacityKey];
	self.handednessControl.selectedSegmentIndex = CTRPadTouchHandednessSteerLeft;
	self.sizeControl.selectedSegmentIndex = CTRPadTouchSizeStandard;
	self.opacityControl.selectedSegmentIndex = CTRPadTouchOpacityStandard;
	[self.overlayController applyPreferencesAndRebuildControls];
	UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, @"Touch controls reset to defaults.");
}

- (void)done
{
	Platform_InputTouchReset();
	[self dismissViewControllerAnimated:YES completion:nil];
}

- (void)viewWillDisappear:(BOOL)animated
{
	[super viewWillDisappear:animated];
	Platform_InputTouchReset();
}

@end

@implementation CTRPadTouchOverlayViewController

- (void)loadView
{
	self.view = [[CTRPadTouchPassthroughView alloc] initWithFrame:CGRectZero];
	self.view.backgroundColor = UIColor.clearColor;
	self.view.multipleTouchEnabled = YES;
	self.view.accessibilityViewIsModal = NO;
}

- (UIButton *)buttonWithTitle:(NSString *)title mask:(enum PlatformInputTouchButton)mask color:(UIColor *)color
{
	UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
	button.translatesAutoresizingMaskIntoConstraints = NO;
	button.tag = mask;
	button.exclusiveTouch = NO;
	button.backgroundColor = [color colorWithAlphaComponent:self.controlOpacity];
	button.layer.borderColor = [UIColor colorWithWhite:1.0 alpha:0.68].CGColor;
	button.layer.borderWidth = 2.0;
	button.titleLabel.font = [UIFont systemFontOfSize:14.0 weight:UIFontWeightBold];
	button.titleLabel.numberOfLines = 2;
	button.titleLabel.textAlignment = NSTextAlignmentCenter;
	[button setTitle:title forState:UIControlStateNormal];
	[button setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
	[button addTarget:self action:@selector(buttonDown:) forControlEvents:UIControlEventTouchDown | UIControlEventTouchDragEnter];
	[button addTarget:self
	              action:@selector(buttonUp:)
	    forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside | UIControlEventTouchCancel | UIControlEventTouchDragExit];
	return button;
}

- (UIButton *)utilityButtonWithTitle:(NSString *)title action:(SEL)action
{
	UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
	button.translatesAutoresizingMaskIntoConstraints = NO;
	button.exclusiveTouch = YES;
	button.backgroundColor = [[UIColor colorWithWhite:0.08 alpha:1.0] colorWithAlphaComponent:self.controlOpacity];
	button.layer.borderColor = [UIColor colorWithWhite:1.0 alpha:0.68].CGColor;
	button.layer.borderWidth = 2.0;
	button.layer.cornerRadius = 12.0;
	button.titleLabel.font = [UIFont systemFontOfSize:12.0 weight:UIFontWeightBold];
	[button setTitle:title forState:UIControlStateNormal];
	[button setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
	[button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
	return button;
}

- (void)buttonDown:(UIButton *)sender
{
	sender.alpha = 0.92;
	Platform_InputTouchButton((unsigned int)sender.tag, 1);
}

- (void)buttonUp:(UIButton *)sender
{
	sender.alpha = 1.0;
	Platform_InputTouchButton((unsigned int)sender.tag, 0);
}

- (void)presentTouchSettings
{
	if (self.presentedViewController != nil)
	{
		return;
	}

	Platform_InputTouchReset();
	CTRPadTouchSettingsViewController *settings = [[CTRPadTouchSettingsViewController alloc] init];
	settings.overlayController = self;
	settings.modalPresentationStyle = UIModalPresentationFormSheet;
	[self presentViewController:settings animated:YES completion:nil];
}

- (void)presentDiscReselectionConfirmation
{
	if ((self.presentedViewController != nil) || (s_discReselectionCallback == NULL))
	{
		return;
	}

	UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Choose a different disc?"
	                                                              message:@"CTRPad must stop the current game before opening Files. Memory-card saves are kept, but unsaved race progress will be lost."
	                                                       preferredStyle:UIAlertControllerStyleAlert];
	[alert addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
	[alert addAction:[UIAlertAction actionWithTitle:@"Stop Game & Choose"
	                                         style:UIAlertActionStyleDefault
	                                       handler:^(__unused UIAlertAction *action) {
	                                         // Allow the alert dismissal transition to finish before the
	                                         // display callback removes the game view hierarchy.
	                                         dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.35 * NSEC_PER_SEC)),
	                                                        dispatch_get_main_queue(), ^{
	                                                          if (s_discReselectionCallback != NULL)
	                                                          {
		                                                          s_discReselectionCallback(s_discReselectionUserdata);
	                                                          }
	                                                        });
	                                       }]];
	alert.preferredAction = alert.actions.lastObject;
	[self presentViewController:alert animated:YES completion:nil];
}

- (void)viewDidLoad
{
	[super viewDidLoad];
	[self applyPreferencesAndRebuildControls];
}

- (void)applyPreferencesAndRebuildControls
{
	Platform_InputTouchReset();
	for (UIView *subview in self.view.subviews.copy)
	{
		[subview removeFromSuperview];
	}

	self.handedness = (CTRPadTouchHandedness)CTRPadTouch_ReadPreference(s_touchHandednessKey,
	                                                                  CTRPadTouchHandednessSteerLeft,
	                                                                  CTRPadTouchHandednessSteerRight);
	CTRPadTouchSize sizeChoice = (CTRPadTouchSize)CTRPadTouch_ReadPreference(s_touchSizeKey,
	                                                                       CTRPadTouchSizeStandard,
	                                                                       CTRPadTouchSizeLarge);
	CTRPadTouchOpacity opacityChoice = (CTRPadTouchOpacity)CTRPadTouch_ReadPreference(s_touchOpacityKey,
	                                                                                CTRPadTouchOpacityStandard,
	                                                                                CTRPadTouchOpacityHigh);
	self.controlScale = CTRPadTouch_ScaleForChoice(sizeChoice);
	self.controlOpacity = CTRPadTouch_OpacityForChoice(opacityChoice);

	CTRPadTouchStickView *stick = [[CTRPadTouchStickView alloc] init];
	[stick applyControlOpacity:self.controlOpacity];
	UIButton *cross = [self buttonWithTitle:@"GAS\n✕" mask:PLATFORM_INPUT_TOUCH_CROSS color:[UIColor colorWithRed:0.08 green:0.42 blue:0.95 alpha:1.0]];
	UIButton *square = [self buttonWithTitle:@"BRAKE\n□" mask:PLATFORM_INPUT_TOUCH_SQUARE color:[UIColor colorWithRed:0.93 green:0.19 blue:0.52 alpha:1.0]];
	UIButton *circle = [self buttonWithTitle:@"ITEM\n○" mask:PLATFORM_INPUT_TOUCH_CIRCLE color:[UIColor colorWithRed:0.93 green:0.16 blue:0.18 alpha:1.0]];
	UIButton *triangle = [self buttonWithTitle:@"VIEW\n△" mask:PLATFORM_INPUT_TOUCH_TRIANGLE color:[UIColor colorWithRed:0.08 green:0.68 blue:0.32 alpha:1.0]];
	UIButton *leftDrift = [self buttonWithTitle:@"L DRIFT / BOOST" mask:PLATFORM_INPUT_TOUCH_L1 color:[UIColor colorWithRed:0.94 green:0.59 blue:0.10 alpha:1.0]];
	UIButton *rightDrift = [self buttonWithTitle:@"R DRIFT / BOOST" mask:PLATFORM_INPUT_TOUCH_R1 color:[UIColor colorWithRed:0.94 green:0.59 blue:0.10 alpha:1.0]];
	UIButton *start = [self buttonWithTitle:@"PAUSE" mask:PLATFORM_INPUT_TOUCH_START color:[UIColor colorWithWhite:0.08 alpha:1.0]];
	UIButton *select = [self buttonWithTitle:@"SELECT" mask:PLATFORM_INPUT_TOUCH_SELECT color:[UIColor colorWithWhite:0.08 alpha:1.0]];
	UIButton *settings = [self utilityButtonWithTitle:@"CONTROLS" action:@selector(presentTouchSettings)];
	UIButton *disc = [self utilityButtonWithTitle:@"CHANGE DISC" action:@selector(presentDiscReselectionConfirmation)];

	cross.accessibilityIdentifier = @"ctrpad.touch.cross";
	square.accessibilityIdentifier = @"ctrpad.touch.square";
	circle.accessibilityIdentifier = @"ctrpad.touch.circle";
	triangle.accessibilityIdentifier = @"ctrpad.touch.triangle";
	leftDrift.accessibilityIdentifier = @"ctrpad.touch.l1";
	rightDrift.accessibilityIdentifier = @"ctrpad.touch.r1";
	start.accessibilityIdentifier = @"ctrpad.touch.start";
	select.accessibilityIdentifier = @"ctrpad.touch.select";
	settings.accessibilityIdentifier = @"ctrpad.touch.settings";
	settings.accessibilityLabel = @"Configure touch controls";
	disc.accessibilityIdentifier = @"ctrpad.touch.disc";
	disc.accessibilityLabel = @"Change retail disc image";

	NSArray<UIView *> *controls = @[ stick, cross, square, circle, triangle, leftDrift, rightDrift, start, select, settings, disc ];
	for (UIView *control in controls)
	{
		[self.view addSubview:control];
	}

	UILayoutGuide *safe = self.view.safeAreaLayoutGuide;
	CGFloat scale = self.controlScale;
	CGFloat stickSize = round(174.0 * scale);
	CGFloat driftWidth = round(144.0 * scale);
	CGFloat driftHeight = MAX(48.0, round(54.0 * scale));
	CGFloat startWidth = round(82.0 * scale);
	CGFloat startHeight = MAX(44.0, round(40.0 * scale));
	CGFloat startCenterOffset = startWidth * 0.5 + 5.0;
	CGFloat utilityWidth = MAX(100.0, round(112.0 * scale));
	CGFloat utilityHeight = 44.0;
	CGFloat utilityCenterOffset = utilityWidth * 0.5 + 5.0;
	CGFloat crossSize = round(92.0 * scale);
	CGFloat faceSize = MAX(56.0, round(64.0 * scale));

	NSMutableArray<NSLayoutConstraint *> *constraints = [@[
		[stick.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor constant:-24.0 * scale],
		[stick.widthAnchor constraintEqualToConstant:stickSize],
		[stick.heightAnchor constraintEqualToAnchor:stick.widthAnchor],

		[leftDrift.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:18.0 * scale],
		[leftDrift.topAnchor constraintEqualToAnchor:safe.topAnchor constant:10.0 * scale],
		[leftDrift.widthAnchor constraintEqualToConstant:driftWidth],
		[leftDrift.heightAnchor constraintEqualToConstant:driftHeight],

		[rightDrift.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-18.0 * scale],
		[rightDrift.topAnchor constraintEqualToAnchor:safe.topAnchor constant:10.0 * scale],
		[rightDrift.widthAnchor constraintEqualToConstant:driftWidth],
		[rightDrift.heightAnchor constraintEqualToConstant:driftHeight],

		[start.topAnchor constraintEqualToAnchor:safe.topAnchor constant:12.0 * scale],
		[start.centerXAnchor constraintEqualToAnchor:safe.centerXAnchor constant:startCenterOffset],
		[start.widthAnchor constraintEqualToConstant:startWidth],
		[start.heightAnchor constraintEqualToConstant:startHeight],
		[select.topAnchor constraintEqualToAnchor:safe.topAnchor constant:12.0 * scale],
		[select.centerXAnchor constraintEqualToAnchor:safe.centerXAnchor constant:-startCenterOffset],
		[select.widthAnchor constraintEqualToConstant:startWidth],
		[select.heightAnchor constraintEqualToConstant:startHeight],

		[settings.topAnchor constraintEqualToAnchor:start.bottomAnchor constant:8.0 * scale],
		[settings.centerXAnchor constraintEqualToAnchor:safe.centerXAnchor constant:-utilityCenterOffset],
		[settings.widthAnchor constraintEqualToConstant:utilityWidth],
		[settings.heightAnchor constraintEqualToConstant:utilityHeight],
		[disc.topAnchor constraintEqualToAnchor:start.bottomAnchor constant:8.0 * scale],
		[disc.centerXAnchor constraintEqualToAnchor:safe.centerXAnchor constant:utilityCenterOffset],
		[disc.widthAnchor constraintEqualToConstant:utilityWidth],
		[disc.heightAnchor constraintEqualToConstant:utilityHeight],

		[cross.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor constant:-22.0 * scale],
		[cross.widthAnchor constraintEqualToConstant:crossSize],
		[cross.heightAnchor constraintEqualToAnchor:cross.widthAnchor],
		[square.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor constant:-34.0 * scale],
		[square.widthAnchor constraintEqualToConstant:faceSize],
		[square.heightAnchor constraintEqualToAnchor:square.widthAnchor],
		[circle.bottomAnchor constraintEqualToAnchor:cross.topAnchor constant:-16.0 * scale],
		[circle.widthAnchor constraintEqualToConstant:faceSize],
		[circle.heightAnchor constraintEqualToAnchor:circle.widthAnchor],
		[triangle.centerYAnchor constraintEqualToAnchor:circle.centerYAnchor],
		[triangle.widthAnchor constraintEqualToConstant:faceSize],
		[triangle.heightAnchor constraintEqualToAnchor:triangle.widthAnchor],
	] mutableCopy];

	if (self.handedness == CTRPadTouchHandednessSteerRight)
	{
		[constraints addObjectsFromArray:@[
			[stick.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-24.0 * scale],
			[cross.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:24.0 * scale],
			[square.leadingAnchor constraintEqualToAnchor:cross.trailingAnchor constant:18.0 * scale],
			[circle.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:36.0 * scale],
			[triangle.leadingAnchor constraintEqualToAnchor:circle.trailingAnchor constant:14.0 * scale],
		]];
	}
	else
	{
		[constraints addObjectsFromArray:@[
			[stick.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:24.0 * scale],
			[cross.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-24.0 * scale],
			[square.trailingAnchor constraintEqualToAnchor:cross.leadingAnchor constant:-18.0 * scale],
			[circle.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-36.0 * scale],
			[triangle.trailingAnchor constraintEqualToAnchor:circle.leadingAnchor constant:-14.0 * scale],
		]];
	}
	[NSLayoutConstraint activateConstraints:constraints];

	for (UIButton *button in @[ cross, square, circle, triangle ])
	{
		button.layer.cornerRadius = button == cross ? crossSize * 0.5 : faceSize * 0.5;
	}
	for (UIButton *button in @[ leftDrift, rightDrift, start, select ])
	{
		button.layer.cornerRadius = 14.0;
	}
}

@end

static UIWindow *CTRPadTouch_FindGameWindow(void)
{
	for (UIScene *scene in UIApplication.sharedApplication.connectedScenes)
	{
		if (![scene isKindOfClass:UIWindowScene.class] || (scene.activationState == UISceneActivationStateUnattached))
		{
			continue;
		}
		for (UIWindow *window in ((UIWindowScene *)scene).windows)
		{
			if (!window.hidden && (window.alpha > 0.0) && (window.windowLevel <= UIWindowLevelNormal + 0.5) &&
			    (window.rootViewController != nil))
			{
				return window;
			}
		}
	}
	return nil;
}

int NativeIOSTouch_Begin(NativeIOSTouchDiscReselectionCallback discReselectionCallback, void *userdata)
{
	if (!NSThread.isMainThread)
	{
		return 0;
	}
	if (s_touchOverlayController != nil)
	{
		return 1;
	}
	s_discReselectionCallback = discReselectionCallback;
	s_discReselectionUserdata = userdata;

	UIWindow *gameWindow = CTRPadTouch_FindGameWindow();
	if (gameWindow == nil)
	{
		s_discReselectionCallback = NULL;
		s_discReselectionUserdata = NULL;
		return 0;
	}

	CTRPadTouchOverlayViewController *controller = [[CTRPadTouchOverlayViewController alloc] init];
	UIViewController *parent = gameWindow.rootViewController;
	UIView *overlay = controller.view;
	[parent addChildViewController:controller];
	overlay.frame = parent.view.bounds;
	overlay.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	[parent.view addSubview:overlay];
	[parent.view bringSubviewToFront:overlay];
	[controller didMoveToParentViewController:parent];
	s_touchOverlayController = controller;
	Platform_InputTouchSetEnabled(1);
	return 1;
}

void NativeIOSTouch_End(void)
{
	if (!NSThread.isMainThread)
	{
		return;
	}
	Platform_InputTouchSetEnabled(0);
	s_discReselectionCallback = NULL;
	s_discReselectionUserdata = NULL;
	[s_touchOverlayController willMoveToParentViewController:nil];
	[s_touchOverlayController.view removeFromSuperview];
	[s_touchOverlayController removeFromParentViewController];
	s_touchOverlayController = nil;
}

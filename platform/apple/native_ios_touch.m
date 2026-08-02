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
static NSString *const s_touchEnabledKey = @"CTRPadTouchEnabled";
static NSString *const s_touchLayoutKeyPrefix = @"CTRPadTouchLayout";
static const NSTimeInterval s_gasLatchDelay = 2.0;

@class CTRPadTouchOverlayViewController;

@interface CTRPadTouchPassthroughView : UIView
@end

@interface CTRPadInputButton : UIButton
@property(nonatomic, assign) BOOL accessibilityHeld;
@property(nonatomic, assign) BOOL accessibilityTapPending;
@property(nonatomic, assign) NSUInteger accessibilityActionGeneration;
@property(nonatomic, assign) BOOL inputPressed;
@property(nonatomic, assign) BOOL inputLatched;
@property(nonatomic, assign) BOOL holdToLatch;
@property(nonatomic, assign) BOOL layoutEditing;
@property(nonatomic, assign) NSUInteger latchGeneration;
@property(nonatomic, strong) UIColor *baseColor;
@property(nonatomic, assign) CGFloat controlOpacity;
- (void)updateAppearance;
- (void)cancelInput;
- (BOOL)accessibilityPressBriefly;
- (BOOL)accessibilityPressThreeSeconds;
- (BOOL)accessibilityHoldControl;
- (BOOL)accessibilityReleaseControl;
@end

@interface CTRPadTouchStickView : UIView
@property(nonatomic, strong) UIView *knob;
@property(nonatomic, strong) UILabel *label;
@property(nonatomic, assign) BOOL trackingTouch;
@property(nonatomic, assign) BOOL accessibilitySteering;
@property(nonatomic, assign) NSUInteger accessibilitySteeringGeneration;
@property(nonatomic, assign) unsigned int directionMask;
@property(nonatomic, assign) BOOL layoutEditing;
- (void)applyControlOpacity:(CGFloat)opacity;
- (void)publishPoint:(CGPoint)point;
- (void)releaseTouch;
- (BOOL)accessibilityHoldLeft;
- (BOOL)accessibilityHoldRight;
- (BOOL)accessibilityNudgeLeft;
- (BOOL)accessibilityNudgeRight;
- (BOOL)accessibilityHoldSlightLeft;
- (BOOL)accessibilityHoldSlightRight;
- (BOOL)accessibilityNudgeSlightLeft;
- (BOOL)accessibilityNudgeSlightRight;
- (BOOL)accessibilityHoldUp;
- (BOOL)accessibilityHoldDown;
- (BOOL)accessibilityCenter;
@end

@interface CTRPadTouchOverlayViewController : UIViewController
@property(nonatomic, assign) CTRPadTouchHandedness handedness;
@property(nonatomic, assign) CGFloat controlScale;
@property(nonatomic, assign) CGFloat controlOpacity;
@property(nonatomic, assign) BOOL touchControlsEnabled;
@property(nonatomic, assign) BOOL layoutEditing;
@property(nonatomic, strong) NSArray<UIView *> *editableControls;
@property(nonatomic, strong) NSMutableArray<UIGestureRecognizer *> *editGestures;
@property(nonatomic, strong) NSMutableDictionary<NSString *, NSArray<NSNumber *> *> *layoutCenters;
@property(nonatomic, strong) NSMutableDictionary<NSString *, NSNumber *> *layoutScales;
@property(nonatomic, copy) NSString *layoutProfile;
@property(nonatomic, weak) UIView *selectedControl;
@property(nonatomic, strong) UIView *editorPanel;
@property(nonatomic, strong) UILabel *editorLabel;
@property(nonatomic, strong) UIButton *editorSmallerButton;
@property(nonatomic, strong) UIButton *editorLargerButton;
@property(nonatomic, strong) UIButton *editorResetButton;
@property(nonatomic, strong) UIButton *editorDoneButton;
- (void)applyPreferencesAndRebuildControls;
- (void)resetControlState;
- (void)beginLayoutEditing;
- (void)endLayoutEditing;
- (void)resetCurrentLayout;
- (void)resetAllLayouts;
- (void)installLayoutEditor;
- (void)layoutControls;
- (void)selectControl:(UIGestureRecognizer *)gesture;
- (void)shrinkSelectedControl;
- (void)growSelectedControl;
@end

@interface CTRPadTouchSettingsViewController : UIViewController
@property(nonatomic, weak) CTRPadTouchOverlayViewController *overlayController;
@property(nonatomic, strong) UISegmentedControl *handednessControl;
@property(nonatomic, strong) UISegmentedControl *sizeControl;
@property(nonatomic, strong) UISegmentedControl *opacityControl;
@property(nonatomic, strong) UISwitch *touchEnabledSwitch;
@property(nonatomic, strong) UIButton *editLayoutButton;
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

@implementation CTRPadInputButton

- (void)updateAppearance
{
	UIColor *color = self.inputLatched
	                     ? [UIColor colorWithRed:0.10 green:0.52 blue:0.98 alpha:1.0]
	                     : (self.baseColor ?: [UIColor colorWithWhite:0.08 alpha:1.0]);
	self.backgroundColor = [color colorWithAlphaComponent:self.inputLatched ? MAX(0.82, self.controlOpacity) : self.controlOpacity];
	self.layer.borderColor = (self.inputLatched
	                              ? [UIColor colorWithRed:0.72 green:0.88 blue:1.0 alpha:1.0]
	                              : [UIColor colorWithWhite:1.0 alpha:0.68])
	                             .CGColor;
	self.alpha = self.inputPressed ? 0.92 : 1.0;
}

- (void)cancelInput
{
	self.accessibilityActionGeneration += 1;
	self.latchGeneration += 1;
	self.accessibilityHeld = NO;
	self.accessibilityTapPending = NO;
	self.inputPressed = NO;
	self.inputLatched = NO;
	self.accessibilityValue = @"Released";
	[self updateAppearance];
}

- (BOOL)accessibilityPressForDuration:(NSTimeInterval)duration value:(NSString *)value
{
	if (self.inputLatched)
	{
		return [self accessibilityReleaseControl];
	}
	[self accessibilityReleaseControl];
	NSUInteger generation = ++self.accessibilityActionGeneration;
	self.accessibilityTapPending = YES;
	self.accessibilityValue = value;
	[self sendActionsForControlEvents:UIControlEventTouchDown];
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(duration * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
		if (self.accessibilityTapPending && !self.accessibilityHeld &&
		    (self.accessibilityActionGeneration == generation))
		{
			self.accessibilityTapPending = NO;
			self.accessibilityValue = self.inputLatched ? @"Gas locked" : @"Released";
			[self sendActionsForControlEvents:UIControlEventTouchUpInside];
			if (self.inputLatched)
			{
				self.accessibilityValue = @"Gas locked";
			}
		}
	});
	return YES;
}

- (BOOL)accessibilityActivate
{
	// Voice Control, Switch Control and Simulator accessibility actions invoke
	// a button's primary action without synthesizing UIControlEventTouchDown.
	// Publish the same down/up pair as a physical finger so accessible controls
	// cannot silently appear to press while the retail pad receives no edge.
	return [self accessibilityPressForDuration:0.10 value:@"Pressed"];
}

- (BOOL)accessibilityPressBriefly
{
	return [self accessibilityPressForDuration:1.0 value:@"Pressed for one second"];
}

- (BOOL)accessibilityPressThreeSeconds
{
	return [self accessibilityPressForDuration:3.0 value:@"Pressed for three seconds"];
}

- (BOOL)accessibilityHoldControl
{
	BOOL wasActive = self.accessibilityHeld || self.accessibilityTapPending;
	self.accessibilityActionGeneration += 1;
	self.accessibilityTapPending = NO;
	self.accessibilityHeld = YES;
	self.accessibilityValue = @"Held";
	if (!wasActive)
	{
		[self sendActionsForControlEvents:UIControlEventTouchDown];
	}
	return YES;
}

- (BOOL)accessibilityReleaseControl
{
	if (self.inputLatched)
	{
		self.accessibilityActionGeneration += 1;
		self.accessibilityHeld = NO;
		self.accessibilityTapPending = NO;
		[self sendActionsForControlEvents:UIControlEventTouchDown];
		return YES;
	}
	BOOL wasActive = self.accessibilityHeld || self.accessibilityTapPending;
	self.accessibilityActionGeneration += 1;
	self.accessibilityHeld = NO;
	self.accessibilityTapPending = NO;
	self.accessibilityValue = @"Released";
	if (wasActive)
	{
		[self sendActionsForControlEvents:UIControlEventTouchUpInside];
	}
	return YES;
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
		self.accessibilityValue = @"Centered";
		self.isAccessibilityElement = YES;
		self.accessibilityCustomActions = @[
			[[UIAccessibilityCustomAction alloc] initWithName:@"Nudge left" target:self selector:@selector(accessibilityNudgeLeft)],
			[[UIAccessibilityCustomAction alloc] initWithName:@"Nudge right" target:self selector:@selector(accessibilityNudgeRight)],
			[[UIAccessibilityCustomAction alloc] initWithName:@"Hold left" target:self selector:@selector(accessibilityHoldLeft)],
			[[UIAccessibilityCustomAction alloc] initWithName:@"Hold right" target:self selector:@selector(accessibilityHoldRight)],
			[[UIAccessibilityCustomAction alloc] initWithName:@"Nudge slight left" target:self selector:@selector(accessibilityNudgeSlightLeft)],
			[[UIAccessibilityCustomAction alloc] initWithName:@"Nudge slight right" target:self selector:@selector(accessibilityNudgeSlightRight)],
			[[UIAccessibilityCustomAction alloc] initWithName:@"Hold slight left" target:self selector:@selector(accessibilityHoldSlightLeft)],
			[[UIAccessibilityCustomAction alloc] initWithName:@"Hold slight right" target:self selector:@selector(accessibilityHoldSlightRight)],
			[[UIAccessibilityCustomAction alloc] initWithName:@"Hold up" target:self selector:@selector(accessibilityHoldUp)],
			[[UIAccessibilityCustomAction alloc] initWithName:@"Hold down" target:self selector:@selector(accessibilityHoldDown)],
			[[UIAccessibilityCustomAction alloc] initWithName:@"Center" target:self selector:@selector(accessibilityCenter)],
		];
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

- (BOOL)accessibilityHoldX:(CGFloat)x y:(CGFloat)y value:(NSString *)value
{
	[self releaseTouch];
	self.accessibilitySteering = YES;
	self.trackingTouch = YES;
	CGPoint center = CGPointMake(CGRectGetMidX(self.bounds), CGRectGetMidY(self.bounds));
	CGFloat radius = MAX(1.0, MIN(self.bounds.size.width, self.bounds.size.height) * 0.5 - self.knob.bounds.size.width * 0.5 - 4.0);
	[self publishPoint:CGPointMake(center.x + x * radius, center.y + y * radius)];
	self.accessibilityValue = value;
	return YES;
}

- (BOOL)accessibilityNudgeX:(CGFloat)x y:(CGFloat)y value:(NSString *)value
{
	[self accessibilityHoldX:x y:y value:value];
	NSUInteger generation = self.accessibilitySteeringGeneration;
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.45 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
		// A newer accessibility action or a physical touch owns the stick now.
		// Do not let an older nudge unexpectedly center that newer input.
		if (self.accessibilitySteering && (self.accessibilitySteeringGeneration == generation))
		{
			[self releaseTouch];
		}
	});
	return YES;
}

- (BOOL)accessibilityHoldLeft
{
	return [self accessibilityHoldX:-1.0 y:0.0 value:@"Held left"];
}

- (BOOL)accessibilityHoldRight
{
	return [self accessibilityHoldX:1.0 y:0.0 value:@"Held right"];
}

- (BOOL)accessibilityNudgeLeft
{
	return [self accessibilityNudgeX:-1.0 y:0.0 value:@"Nudging left"];
}

- (BOOL)accessibilityNudgeRight
{
	return [self accessibilityNudgeX:1.0 y:0.0 value:@"Nudging right"];
}

- (BOOL)accessibilityHoldSlightLeft
{
	return [self accessibilityHoldX:-0.45 y:0.0 value:@"Held slight left"];
}

- (BOOL)accessibilityHoldSlightRight
{
	return [self accessibilityHoldX:0.45 y:0.0 value:@"Held slight right"];
}

- (BOOL)accessibilityNudgeSlightLeft
{
	return [self accessibilityNudgeX:-0.45 y:0.0 value:@"Nudging slight left"];
}

- (BOOL)accessibilityNudgeSlightRight
{
	return [self accessibilityNudgeX:0.45 y:0.0 value:@"Nudging slight right"];
}

- (BOOL)accessibilityHoldUp
{
	return [self accessibilityHoldX:0.0 y:-1.0 value:@"Held up"];
}

- (BOOL)accessibilityHoldDown
{
	return [self accessibilityHoldX:0.0 y:1.0 value:@"Held down"];
}

- (BOOL)accessibilityCenter
{
	[self releaseTouch];
	return YES;
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
	if (self.layoutEditing)
	{
		return;
	}
	UITouch *touch = touches.anyObject;
	if (touch == nil)
	{
		return;
	}
	if (self.accessibilitySteering)
	{
		[self releaseTouch];
	}
	self.trackingTouch = YES;
	[self publishPoint:[touch locationInView:self]];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
	(void)event;
	if (self.layoutEditing)
	{
		return;
	}
	UITouch *touch = touches.anyObject;
	if ((touch != nil) && self.trackingTouch)
	{
		[self publishPoint:[touch locationInView:self]];
	}
}

- (void)releaseTouch
{
	self.accessibilitySteeringGeneration += 1;
	self.trackingTouch = NO;
	self.accessibilitySteering = NO;
	self.accessibilityValue = @"Centered";
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
	bodyLabel.text = @"Turn gameplay controls on or off, or move and resize them in Edit Layout. Hold Gas for two seconds to lock it, then tap Gas to release.";
	bodyLabel.textColor = [UIColor colorWithWhite:0.72 alpha:1.0];
	bodyLabel.font = [UIFont systemFontOfSize:15.0 weight:UIFontWeightRegular];
	bodyLabel.textAlignment = NSTextAlignmentCenter;
	bodyLabel.numberOfLines = 0;

	UILabel *touchEnabledLabel = [self sectionLabelWithText:@"On-screen controls"];
	self.touchEnabledSwitch = [[UISwitch alloc] init];
	self.touchEnabledSwitch.on = CTRPadTouch_ReadPreference(s_touchEnabledKey, 1, 1) != 0;
	self.touchEnabledSwitch.accessibilityIdentifier = @"ctrpad.touch.settings.enabled";
	self.touchEnabledSwitch.accessibilityLabel = @"On-screen touch controls";
	[self.touchEnabledSwitch addTarget:self action:@selector(preferencesChanged) forControlEvents:UIControlEventValueChanged];
	UIStackView *touchEnabledRow = [[UIStackView alloc] initWithArrangedSubviews:@[ touchEnabledLabel, self.touchEnabledSwitch ]];
	touchEnabledRow.axis = UILayoutConstraintAxisHorizontal;
	touchEnabledRow.alignment = UIStackViewAlignmentCenter;
	touchEnabledRow.distribution = UIStackViewDistributionEqualSpacing;

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

	UIButtonConfiguration *editConfiguration = [UIButtonConfiguration tintedButtonConfiguration];
	editConfiguration.title = @"Edit touch layout";
	editConfiguration.baseForegroundColor = [UIColor colorWithRed:0.34 green:0.70 blue:1.0 alpha:1.0];
	self.editLayoutButton = [UIButton buttonWithConfiguration:editConfiguration primaryAction:nil];
	self.editLayoutButton.accessibilityIdentifier = @"ctrpad.touch.settings.edit-layout";
	self.editLayoutButton.enabled = self.touchEnabledSwitch.on;
	[self.editLayoutButton addTarget:self action:@selector(editLayout) forControlEvents:UIControlEventTouchUpInside];

	UIButtonConfiguration *doneConfiguration = [UIButtonConfiguration filledButtonConfiguration];
	doneConfiguration.title = @"Done";
	doneConfiguration.baseBackgroundColor = [UIColor colorWithRed:0.13 green:0.42 blue:0.84 alpha:1.0];
	UIButton *doneButton = [UIButton buttonWithConfiguration:doneConfiguration primaryAction:nil];
	doneButton.accessibilityIdentifier = @"ctrpad.touch.settings.done";
	[doneButton addTarget:self action:@selector(done) forControlEvents:UIControlEventTouchUpInside];

	UIStackView *stack = [[UIStackView alloc] initWithArrangedSubviews:@[
		titleLabel,
		bodyLabel,
		touchEnabledRow,
		[self sectionLabelWithText:@"Handedness"],
		self.handednessControl,
		[self sectionLabelWithText:@"Control size"],
		self.sizeControl,
		[self sectionLabelWithText:@"Control opacity"],
		self.opacityControl,
		keyboardLabel,
		keyboardMap,
		self.editLayoutButton,
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
		[self.editLayoutButton.heightAnchor constraintGreaterThanOrEqualToConstant:48.0],
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
	[defaults setBool:self.touchEnabledSwitch.on forKey:s_touchEnabledKey];
	self.editLayoutButton.enabled = self.touchEnabledSwitch.on;
	[self.overlayController applyPreferencesAndRebuildControls];
}

- (void)resetDefaults
{
	NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
	[defaults removeObjectForKey:s_touchHandednessKey];
	[defaults removeObjectForKey:s_touchSizeKey];
	[defaults removeObjectForKey:s_touchOpacityKey];
	[defaults removeObjectForKey:s_touchEnabledKey];
	self.handednessControl.selectedSegmentIndex = CTRPadTouchHandednessSteerLeft;
	self.sizeControl.selectedSegmentIndex = CTRPadTouchSizeStandard;
	self.opacityControl.selectedSegmentIndex = CTRPadTouchOpacityStandard;
	self.touchEnabledSwitch.on = YES;
	self.editLayoutButton.enabled = YES;
	[self.overlayController resetAllLayouts];
	[self.overlayController applyPreferencesAndRebuildControls];
	UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, @"Touch controls reset to defaults.");
}

- (void)editLayout
{
	if (!self.touchEnabledSwitch.on)
	{
		return;
	}
	[self.overlayController resetControlState];
	__weak CTRPadTouchOverlayViewController *overlay = self.overlayController;
	[self dismissViewControllerAnimated:YES completion:^{
		[overlay beginLayoutEditing];
	}];
}

- (void)done
{
	[self.overlayController resetControlState];
	[self dismissViewControllerAnimated:YES completion:nil];
}

- (void)viewWillDisappear:(BOOL)animated
{
	[super viewWillDisappear:animated];
	[self.overlayController resetControlState];
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

- (CTRPadInputButton *)buttonWithTitle:(NSString *)title mask:(enum PlatformInputTouchButton)mask color:(UIColor *)color
{
	CTRPadInputButton *button = [CTRPadInputButton buttonWithType:UIButtonTypeCustom];
	button.translatesAutoresizingMaskIntoConstraints = NO;
	button.tag = mask;
	button.exclusiveTouch = NO;
	button.baseColor = color;
	button.controlOpacity = self.controlOpacity;
	[button updateAppearance];
	button.layer.borderColor = [UIColor colorWithWhite:1.0 alpha:0.68].CGColor;
	button.layer.borderWidth = 2.0;
	button.titleLabel.font = [UIFont systemFontOfSize:14.0 weight:UIFontWeightBold];
	button.titleLabel.numberOfLines = 2;
	button.titleLabel.textAlignment = NSTextAlignmentCenter;
	[button setTitle:title forState:UIControlStateNormal];
	[button setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
	button.accessibilityValue = @"Released";
	button.accessibilityCustomActions = @[
		[[UIAccessibilityCustomAction alloc] initWithName:@"Press one second" target:button selector:@selector(accessibilityPressBriefly)],
		[[UIAccessibilityCustomAction alloc] initWithName:@"Press three seconds" target:button selector:@selector(accessibilityPressThreeSeconds)],
		[[UIAccessibilityCustomAction alloc] initWithName:@"Hold" target:button selector:@selector(accessibilityHoldControl)],
		[[UIAccessibilityCustomAction alloc] initWithName:@"Release" target:button selector:@selector(accessibilityReleaseControl)],
	];
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
	CTRPadInputButton *button = [sender isKindOfClass:CTRPadInputButton.class] ? (CTRPadInputButton *)sender : nil;
	if ((button == nil) || button.layoutEditing)
	{
		return;
	}
	if (button.inputLatched)
	{
		button.latchGeneration += 1;
		button.inputLatched = NO;
		button.inputPressed = NO;
		button.accessibilityValue = @"Released";
		[button updateAppearance];
		Platform_InputTouchButton((unsigned int)button.tag, 0);
		UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, @"Gas lock released.");
		return;
	}
	if (button.inputPressed)
	{
		return;
	}

	button.inputPressed = YES;
	button.accessibilityValue = @"Held";
	[button updateAppearance];
	Platform_InputTouchButton((unsigned int)button.tag, 1);
	if (button.holdToLatch)
	{
		NSUInteger generation = ++button.latchGeneration;
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(s_gasLatchDelay * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
			if ((button.latchGeneration == generation) && button.inputPressed && !button.layoutEditing)
			{
				button.inputLatched = YES;
				button.accessibilityValue = @"Gas locked";
				[button updateAppearance];
				UIImpactFeedbackGenerator *feedback = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium];
				[feedback impactOccurred];
				UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, @"Gas locked. Tap Gas to release.");
			}
		});
	}
}

- (void)buttonUp:(UIButton *)sender
{
	if (![sender isKindOfClass:CTRPadInputButton.class])
	{
		return;
	}
	CTRPadInputButton *button = (CTRPadInputButton *)sender;
	button.latchGeneration += 1;
	button.accessibilityHeld = NO;
	button.accessibilityTapPending = NO;
	if (button.layoutEditing || button.inputLatched || !button.inputPressed)
	{
		return;
	}
	button.inputPressed = NO;
	button.accessibilityValue = @"Released";
	[button updateAppearance];
	Platform_InputTouchButton((unsigned int)button.tag, 0);
}

- (void)resetControlState
{
	for (UIView *control in self.view.subviews)
	{
		if ([control isKindOfClass:CTRPadInputButton.class])
		{
			CTRPadInputButton *button = (CTRPadInputButton *)control;
			[button cancelInput];
		}
		else if ([control isKindOfClass:CTRPadTouchStickView.class])
		{
			[(CTRPadTouchStickView *)control releaseTouch];
		}
	}
	Platform_InputTouchReset();
}

- (void)presentTouchSettings
{
	if (self.presentedViewController != nil)
	{
		return;
	}

	[self resetControlState];
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
	[self resetControlState];

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
	NSNotificationCenter *notifications = NSNotificationCenter.defaultCenter;
	[notifications addObserver:self selector:@selector(lifecycleWillSuspend:) name:UIApplicationWillResignActiveNotification object:nil];
	[notifications addObserver:self selector:@selector(lifecycleWillSuspend:) name:UIApplicationDidEnterBackgroundNotification object:nil];
	[self applyPreferencesAndRebuildControls];
}

- (void)dealloc
{
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)lifecycleWillSuspend:(NSNotification *)notification
{
	(void)notification;
	[self resetControlState];
}

- (void)viewDidLayoutSubviews
{
	[super viewDidLayoutSubviews];
	[self layoutControls];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
	[self resetControlState];
	[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

- (void)applyPreferencesAndRebuildControls
{
	[self resetControlState];
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
	self.touchControlsEnabled = CTRPadTouch_ReadPreference(s_touchEnabledKey, 1, 1) != 0;

	CTRPadTouchStickView *stick = [[CTRPadTouchStickView alloc] init];
	[stick applyControlOpacity:self.controlOpacity];
	CTRPadInputButton *cross = [self buttonWithTitle:@"GAS\n✕" mask:PLATFORM_INPUT_TOUCH_CROSS color:[UIColor colorWithRed:0.08 green:0.42 blue:0.95 alpha:1.0]];
	CTRPadInputButton *square = [self buttonWithTitle:@"BRAKE\n□" mask:PLATFORM_INPUT_TOUCH_SQUARE color:[UIColor colorWithRed:0.93 green:0.19 blue:0.52 alpha:1.0]];
	CTRPadInputButton *circle = [self buttonWithTitle:@"ITEM\n○" mask:PLATFORM_INPUT_TOUCH_CIRCLE color:[UIColor colorWithRed:0.93 green:0.16 blue:0.18 alpha:1.0]];
	CTRPadInputButton *triangle = [self buttonWithTitle:@"VIEW\n△" mask:PLATFORM_INPUT_TOUCH_TRIANGLE color:[UIColor colorWithRed:0.08 green:0.68 blue:0.32 alpha:1.0]];
	CTRPadInputButton *leftDrift = [self buttonWithTitle:@"L DRIFT / BOOST" mask:PLATFORM_INPUT_TOUCH_L1 color:[UIColor colorWithRed:0.94 green:0.59 blue:0.10 alpha:1.0]];
	CTRPadInputButton *rightDrift = [self buttonWithTitle:@"R DRIFT / BOOST" mask:PLATFORM_INPUT_TOUCH_R1 color:[UIColor colorWithRed:0.94 green:0.59 blue:0.10 alpha:1.0]];
	CTRPadInputButton *start = [self buttonWithTitle:@"START\nPAUSE" mask:PLATFORM_INPUT_TOUCH_START color:[UIColor colorWithWhite:0.08 alpha:1.0]];
	CTRPadInputButton *select = [self buttonWithTitle:@"SELECT" mask:PLATFORM_INPUT_TOUCH_SELECT color:[UIColor colorWithWhite:0.08 alpha:1.0]];
	UIButton *settings = [self utilityButtonWithTitle:@"CONTROLS" action:@selector(presentTouchSettings)];
	UIButton *disc = [self utilityButtonWithTitle:@"CHANGE DISC" action:@selector(presentDiscReselectionConfirmation)];
	cross.holdToLatch = YES;

	cross.accessibilityIdentifier = @"ctrpad.touch.cross";
	square.accessibilityIdentifier = @"ctrpad.touch.square";
	circle.accessibilityIdentifier = @"ctrpad.touch.circle";
	triangle.accessibilityIdentifier = @"ctrpad.touch.triangle";
	leftDrift.accessibilityIdentifier = @"ctrpad.touch.l1";
	rightDrift.accessibilityIdentifier = @"ctrpad.touch.r1";
	start.accessibilityIdentifier = @"ctrpad.touch.start";
	start.accessibilityLabel = @"Start or pause";
	select.accessibilityIdentifier = @"ctrpad.touch.select";
	settings.accessibilityIdentifier = @"ctrpad.touch.settings";
	settings.accessibilityLabel = @"Configure touch controls";
	disc.accessibilityIdentifier = @"ctrpad.touch.disc";
	disc.accessibilityLabel = @"Change retail disc image";
	cross.accessibilityLabel = @"Gas, PlayStation Cross";
	square.accessibilityLabel = @"Brake, PlayStation Square";
	circle.accessibilityLabel = @"Item, PlayStation Circle";
	triangle.accessibilityLabel = @"View, PlayStation Triangle";
	leftDrift.accessibilityLabel = @"Left drift or boost, PlayStation L1";
	rightDrift.accessibilityLabel = @"Right drift or boost, PlayStation R1";
	select.accessibilityLabel = @"PlayStation Select";

	NSArray<UIView *> *controls = @[ stick, cross, square, circle, triangle, leftDrift, rightDrift, start, select, settings, disc ];
	for (UIView *control in controls)
	{
		control.translatesAutoresizingMaskIntoConstraints = YES;
		[self.view addSubview:control];
	}
	self.editableControls = @[ stick, cross, square, circle, triangle, leftDrift, rightDrift, start, select ];
	for (UIView *control in self.editableControls)
	{
		control.hidden = !self.touchControlsEnabled;
		control.userInteractionEnabled = self.touchControlsEnabled;
	}
	Platform_InputTouchSetEnabled(self.touchControlsEnabled ? 1 : 0);
	self.editGestures = [NSMutableArray array];
	self.layoutCenters = [NSMutableDictionary dictionary];
	self.layoutScales = [NSMutableDictionary dictionary];
	self.layoutProfile = nil;
	self.selectedControl = nil;
	[self installLayoutEditor];
	[self.view setNeedsLayout];
}

- (CGRect)usableBounds
{
	CGRect safe = UIEdgeInsetsInsetRect(self.view.bounds, self.view.safeAreaInsets);
	return CGRectInset(safe, 4.0, 4.0);
}

- (UIView *)controlWithIdentifier:(NSString *)identifier
{
	for (UIView *view in self.view.subviews)
	{
		if ([view.accessibilityIdentifier isEqualToString:identifier])
		{
			return view;
		}
	}
	return nil;
}

- (NSString *)currentLayoutProfile
{
	NSString *device = UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad ? @"tablet" : @"phone";
	NSString *grip = self.handedness == CTRPadTouchHandednessSteerRight ? @"right" : @"left";
	return [NSString stringWithFormat:@"%@-%@-v1", device, grip];
}

- (NSString *)layoutStorageKey
{
	return [NSString stringWithFormat:@"%@.%@", s_touchLayoutKeyPrefix, self.layoutProfile ?: [self currentLayoutProfile]];
}

- (void)loadCurrentLayoutIfNeeded
{
	NSString *profile = [self currentLayoutProfile];
	if ([self.layoutProfile isEqualToString:profile])
	{
		return;
	}
	self.layoutProfile = profile;
	[self.layoutCenters removeAllObjects];
	[self.layoutScales removeAllObjects];
	NSDictionary *stored = [NSUserDefaults.standardUserDefaults dictionaryForKey:[self layoutStorageKey]];
	NSDictionary *centers = [stored isKindOfClass:NSDictionary.class] ? stored[@"centers"] : nil;
	if ([centers isKindOfClass:NSDictionary.class])
	{
		[centers enumerateKeysAndObjectsUsingBlock:^(NSString *key, id value, __unused BOOL *stop) {
			if ([key isKindOfClass:NSString.class] && [value isKindOfClass:NSArray.class] && [(NSArray *)value count] == 2)
			{
				self.layoutCenters[key] = value;
			}
		}];
	}
	NSDictionary *scales = [stored isKindOfClass:NSDictionary.class] ? stored[@"scales"] : nil;
	if ([scales isKindOfClass:NSDictionary.class])
	{
		[scales enumerateKeysAndObjectsUsingBlock:^(NSString *key, id value, __unused BOOL *stop) {
			CGFloat scale = [value isKindOfClass:NSNumber.class] ? [value doubleValue] : 0.0;
			if ([key isKindOfClass:NSString.class] && isfinite(scale) && (scale >= 0.70) && (scale <= 1.50))
			{
				self.layoutScales[key] = @(scale);
			}
		}];
	}
}

- (void)clampControlToUsableBounds:(UIView *)control
{
	CGRect safe = [self usableBounds];
	CGFloat halfWidth = CGRectGetWidth(control.bounds) * 0.5;
	CGFloat halfHeight = CGRectGetHeight(control.bounds) * 0.5;
	CGFloat minX = CGRectGetMinX(safe) + halfWidth;
	CGFloat maxX = CGRectGetMaxX(safe) - halfWidth;
	CGFloat minY = CGRectGetMinY(safe) + halfHeight;
	CGFloat maxY = CGRectGetMaxY(safe) - halfHeight;
	control.center = CGPointMake(MIN(MAX(control.center.x, minX), MAX(minX, maxX)),
	                             MIN(MAX(control.center.y, minY), MAX(minY, maxY)));
}

- (void)setControl:(UIView *)control normalizedCenter:(CGPoint)normalized size:(CGSize)size mirror:(BOOL)mirror
{
	if (control == nil)
	{
		return;
	}
	CGRect safe = [self usableBounds];
	if (mirror && (self.handedness == CTRPadTouchHandednessSteerRight))
	{
		normalized.x = 1.0 - normalized.x;
	}
	control.bounds = CGRectMake(0.0, 0.0, round(size.width), round(size.height));
	control.center = CGPointMake(CGRectGetMinX(safe) + normalized.x * CGRectGetWidth(safe),
	                             CGRectGetMinY(safe) + normalized.y * CGRectGetHeight(safe));
	NSArray<NSNumber *> *saved = self.layoutCenters[control.accessibilityIdentifier];
	if ([saved isKindOfClass:NSArray.class] && (saved.count == 2))
	{
		CGFloat x = saved[0].doubleValue;
		CGFloat y = saved[1].doubleValue;
		if (isfinite(x) && isfinite(y))
		{
			control.center = CGPointMake(CGRectGetMinX(safe) + x * CGRectGetWidth(safe),
			                             CGRectGetMinY(safe) + y * CGRectGetHeight(safe));
		}
	}
	[self clampControlToUsableBounds:control];
}

- (CGSize)sizeForControl:(NSString *)identifier baseSize:(CGSize)baseSize
{
	CGFloat individualScale = [self.layoutScales[identifier] doubleValue];
	if (!isfinite(individualScale) || (individualScale < 0.70) || (individualScale > 1.50))
		individualScale = 1.0;
	return CGSizeMake(baseSize.width * individualScale, baseSize.height * individualScale);
}

- (void)layoutControls
{
	if ((CGRectGetWidth(self.view.bounds) <= 0.0) || (CGRectGetHeight(self.view.bounds) <= 0.0) || (self.editableControls.count == 0))
	{
		return;
	}
	[self loadCurrentLayoutIfNeeded];
	BOOL phone = UIDevice.currentDevice.userInterfaceIdiom != UIUserInterfaceIdiomPad;
	CGRect safe = [self usableBounds];
	CGFloat scale = self.controlScale;
	CGSize stickSize = [self sizeForControl:@"ctrpad.touch.stick" baseSize:CGSizeMake(174.0 * scale, 174.0 * scale)];
	CGSize driftSize = [self sizeForControl:@"ctrpad.touch.l1" baseSize:CGSizeMake(144.0 * scale, MAX(48.0, 54.0 * scale))];
	CGSize rightDriftSize = [self sizeForControl:@"ctrpad.touch.r1" baseSize:CGSizeMake(144.0 * scale, MAX(48.0, 54.0 * scale))];
	CGSize gasSize = [self sizeForControl:@"ctrpad.touch.cross" baseSize:CGSizeMake(92.0 * scale, 92.0 * scale)];
	CGSize brakeSize = [self sizeForControl:@"ctrpad.touch.square" baseSize:CGSizeMake(MAX(56.0, 64.0 * scale), MAX(56.0, 64.0 * scale))];
	CGSize itemSize = [self sizeForControl:@"ctrpad.touch.circle" baseSize:CGSizeMake(MAX(56.0, 64.0 * scale), MAX(56.0, 64.0 * scale))];
	CGSize viewSize = [self sizeForControl:@"ctrpad.touch.triangle" baseSize:CGSizeMake(MAX(56.0, 64.0 * scale), MAX(56.0, 64.0 * scale))];
	CGSize startSize = [self sizeForControl:@"ctrpad.touch.start" baseSize:CGSizeMake(82.0 * scale, MAX(44.0, 40.0 * scale))];
	CGSize selectSize = [self sizeForControl:@"ctrpad.touch.select" baseSize:CGSizeMake(82.0 * scale, MAX(44.0, 40.0 * scale))];

	CGFloat safeWidth = MAX(1.0, CGRectGetWidth(safe));
	CGFloat safeHeight = MAX(1.0, CGRectGetHeight(safe));
	CGPoint stickPoint = CGPointMake(CGRectGetMinX(safe) + 24.0 * scale + stickSize.width * 0.5,
	                                CGRectGetMaxY(safe) - 24.0 * scale - stickSize.height * 0.5);
	CGPoint gasPoint = CGPointMake(CGRectGetMaxX(safe) - 24.0 * scale - gasSize.width * 0.5,
	                              CGRectGetMaxY(safe) - 22.0 * scale - gasSize.height * 0.5);
	CGPoint brakePoint = CGPointMake(gasPoint.x - gasSize.width * 0.5 - 18.0 * scale - brakeSize.width * 0.5,
	                                CGRectGetMaxY(safe) - 34.0 * scale - brakeSize.height * 0.5);
	CGPoint itemPoint = CGPointMake(CGRectGetMaxX(safe) - 36.0 * scale - itemSize.width * 0.5,
	                               gasPoint.y - gasSize.height * 0.5 - 16.0 * scale - itemSize.height * 0.5);
	CGPoint viewPoint = CGPointMake(itemPoint.x - itemSize.width * 0.5 - 14.0 * scale - viewSize.width * 0.5,
	                               itemPoint.y);
	CGFloat utilityClusterX = viewPoint.x - viewSize.width * 0.5 - 14.0 * scale - MAX(startSize.width, selectSize.width) * 0.5;
	CGPoint startPoint = CGPointMake(utilityClusterX, viewPoint.y - startSize.height * 0.5 - 8.0 * scale);
	CGPoint selectPoint = CGPointMake(utilityClusterX, viewPoint.y + selectSize.height * 0.5 + 8.0 * scale);
	if (self.handedness == CTRPadTouchHandednessSteerRight)
	{
		CGPoint *mirroredPoints[] = { &stickPoint, &gasPoint, &brakePoint, &itemPoint, &viewPoint, &startPoint, &selectPoint };
		for (NSUInteger index = 0; index < sizeof(mirroredPoints) / sizeof(mirroredPoints[0]); ++index)
			mirroredPoints[index]->x = CGRectGetMinX(safe) + CGRectGetMaxX(safe) - mirroredPoints[index]->x;
	}
	CGFloat actionClusterTop = MIN(startPoint.y - startSize.height * 0.5,
	                               MIN(viewPoint.y - viewSize.height * 0.5, itemPoint.y - itemSize.height * 0.5));
	CGFloat driftGroupY = actionClusterTop - 12.0 * scale - MAX(driftSize.height, rightDriftSize.height) * 0.5;
	CGPoint rightDriftPoint = CGPointMake(CGRectGetMaxX(safe) - 18.0 * scale - rightDriftSize.width * 0.5,
	                                     driftGroupY);
	CGPoint leftDriftPoint = CGPointMake(rightDriftPoint.x - rightDriftSize.width * 0.5 - 10.0 * scale - driftSize.width * 0.5,
	                                    driftGroupY);
	if (self.handedness == CTRPadTouchHandednessSteerRight)
	{
		leftDriftPoint.x = CGRectGetMinX(safe) + CGRectGetMaxX(safe) - leftDriftPoint.x;
		rightDriftPoint.x = CGRectGetMinX(safe) + CGRectGetMaxX(safe) - rightDriftPoint.x;
	}

#define CTRPAD_NORMALIZED_POINT(point) CGPointMake(((point).x - CGRectGetMinX(safe)) / safeWidth, ((point).y - CGRectGetMinY(safe)) / safeHeight)
	[self setControl:[self controlWithIdentifier:@"ctrpad.touch.stick"] normalizedCenter:CTRPAD_NORMALIZED_POINT(stickPoint) size:stickSize mirror:NO];
	[self setControl:[self controlWithIdentifier:@"ctrpad.touch.cross"] normalizedCenter:CTRPAD_NORMALIZED_POINT(gasPoint) size:gasSize mirror:NO];
	[self setControl:[self controlWithIdentifier:@"ctrpad.touch.square"] normalizedCenter:CTRPAD_NORMALIZED_POINT(brakePoint) size:brakeSize mirror:NO];
	[self setControl:[self controlWithIdentifier:@"ctrpad.touch.circle"] normalizedCenter:CTRPAD_NORMALIZED_POINT(itemPoint) size:itemSize mirror:NO];
	[self setControl:[self controlWithIdentifier:@"ctrpad.touch.triangle"] normalizedCenter:CTRPAD_NORMALIZED_POINT(viewPoint) size:viewSize mirror:NO];
	[self setControl:[self controlWithIdentifier:@"ctrpad.touch.l1"] normalizedCenter:CTRPAD_NORMALIZED_POINT(leftDriftPoint) size:driftSize mirror:NO];
	[self setControl:[self controlWithIdentifier:@"ctrpad.touch.r1"] normalizedCenter:CTRPAD_NORMALIZED_POINT(rightDriftPoint) size:rightDriftSize mirror:NO];
	[self setControl:[self controlWithIdentifier:@"ctrpad.touch.start"] normalizedCenter:CTRPAD_NORMALIZED_POINT(startPoint) size:startSize mirror:NO];
	[self setControl:[self controlWithIdentifier:@"ctrpad.touch.select"] normalizedCenter:CTRPAD_NORMALIZED_POINT(selectPoint) size:selectSize mirror:NO];
#undef CTRPAD_NORMALIZED_POINT

	for (UIView *control in self.editableControls)
	{
		if ([control isKindOfClass:CTRPadInputButton.class])
		{
			CTRPadInputButton *button = (CTRPadInputButton *)control;
			button.controlOpacity = self.layoutEditing ? 0.90 : self.controlOpacity;
			[button updateAppearance];
			button.layer.cornerRadius = [button.accessibilityIdentifier isEqualToString:@"ctrpad.touch.cross"] ||
			                                    [button.accessibilityIdentifier isEqualToString:@"ctrpad.touch.square"] ||
			                                    [button.accessibilityIdentifier isEqualToString:@"ctrpad.touch.circle"] ||
			                                    [button.accessibilityIdentifier isEqualToString:@"ctrpad.touch.triangle"]
			                                ? MIN(CGRectGetWidth(button.bounds), CGRectGetHeight(button.bounds)) * 0.5
			                                : 14.0;
		}
		else if ([control isKindOfClass:CTRPadTouchStickView.class])
		{
			[(CTRPadTouchStickView *)control applyControlOpacity:self.layoutEditing ? 0.90 : self.controlOpacity];
		}
	}

	UIView *settings = [self controlWithIdentifier:@"ctrpad.touch.settings"];
	UIView *disc = [self controlWithIdentifier:@"ctrpad.touch.disc"];
	CGFloat utilityWidth = phone ? 102.0 : 112.0;
	CGFloat utilityHeight = 44.0;
	settings.bounds = CGRectMake(0.0, 0.0, utilityWidth, utilityHeight);
	disc.bounds = CGRectMake(0.0, 0.0, utilityWidth, utilityHeight);
	disc.center = CGPointMake(CGRectGetMaxX(safe) - utilityWidth * 0.5, CGRectGetMinY(safe) + utilityHeight * 0.5 + 4.0);
	settings.center = CGPointMake(CGRectGetMinX(disc.frame) - 10.0 - utilityWidth * 0.5, disc.center.y);
	settings.hidden = self.layoutEditing;
	disc.hidden = self.layoutEditing;

	if (self.layoutEditing)
	{
		CGFloat panelWidth = MIN(CGRectGetWidth(safe), phone ? 680.0 : 720.0);
		CGFloat panelHeight = 54.0;
		self.editorPanel.frame = CGRectMake(CGRectGetMidX(safe) - panelWidth * 0.5, CGRectGetMinY(safe), panelWidth, panelHeight);
		self.editorLabel.frame = CGRectMake(12.0, 4.0, MAX(110.0, panelWidth - 304.0), panelHeight - 8.0);
		CGFloat actionsX = panelWidth - 286.0;
		self.editorSmallerButton.frame = CGRectMake(actionsX, 7.0, 44.0, 40.0);
		self.editorLargerButton.frame = CGRectMake(actionsX + 50.0, 7.0, 44.0, 40.0);
		self.editorResetButton.frame = CGRectMake(actionsX + 102.0, 7.0, 78.0, 40.0);
		self.editorDoneButton.frame = CGRectMake(actionsX + 188.0, 7.0, 88.0, 40.0);
		[self.view bringSubviewToFront:self.editorPanel];
	}
}

- (UIButton *)editorButtonWithTitle:(NSString *)title action:(SEL)action
{
	UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
	[button setTitle:title forState:UIControlStateNormal];
	[button setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
	button.titleLabel.font = [UIFont systemFontOfSize:14.0 weight:UIFontWeightBold];
	button.backgroundColor = [UIColor colorWithWhite:1.0 alpha:0.16];
	button.layer.cornerRadius = 10.0;
	[button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
	return button;
}

- (void)installLayoutEditor
{
	self.editorPanel = [[UIView alloc] initWithFrame:CGRectZero];
	self.editorPanel.backgroundColor = [UIColor colorWithRed:0.025 green:0.035 blue:0.075 alpha:0.94];
	self.editorPanel.layer.borderColor = [UIColor colorWithWhite:1.0 alpha:0.30].CGColor;
	self.editorPanel.layer.borderWidth = 1.0;
	self.editorPanel.layer.cornerRadius = 14.0;
	self.editorPanel.hidden = YES;
	self.editorPanel.accessibilityIdentifier = @"ctrpad.touch.editor";

	self.editorLabel = [[UILabel alloc] initWithFrame:CGRectZero];
	self.editorLabel.text = @"Tap or drag a control • − / + resizes";
	self.editorLabel.textColor = UIColor.whiteColor;
	self.editorLabel.font = [UIFont systemFontOfSize:13.0 weight:UIFontWeightSemibold];
	self.editorLabel.numberOfLines = 2;
	[self.editorPanel addSubview:self.editorLabel];
	self.editorSmallerButton = [self editorButtonWithTitle:@"−" action:@selector(shrinkSelectedControl)];
	self.editorSmallerButton.accessibilityLabel = @"Make selected control smaller";
	self.editorSmallerButton.accessibilityIdentifier = @"ctrpad.touch.editor.smaller";
	self.editorLargerButton = [self editorButtonWithTitle:@"+" action:@selector(growSelectedControl)];
	self.editorLargerButton.accessibilityLabel = @"Make selected control larger";
	self.editorLargerButton.accessibilityIdentifier = @"ctrpad.touch.editor.larger";
	self.editorResetButton = [self editorButtonWithTitle:@"Reset" action:@selector(resetCurrentLayout)];
	self.editorResetButton.accessibilityIdentifier = @"ctrpad.touch.editor.reset";
	self.editorDoneButton = [self editorButtonWithTitle:@"Done" action:@selector(endLayoutEditing)];
	self.editorDoneButton.backgroundColor = [UIColor colorWithRed:0.13 green:0.42 blue:0.84 alpha:1.0];
	self.editorDoneButton.accessibilityIdentifier = @"ctrpad.touch.editor.done";
	[self.editorPanel addSubview:self.editorSmallerButton];
	[self.editorPanel addSubview:self.editorLargerButton];
	[self.editorPanel addSubview:self.editorResetButton];
	[self.editorPanel addSubview:self.editorDoneButton];
	[self.view addSubview:self.editorPanel];

	for (UIView *control in self.editableControls)
	{
		UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(moveControl:)];
		pan.enabled = NO;
		pan.cancelsTouchesInView = YES;
		[control addGestureRecognizer:pan];
		[self.editGestures addObject:pan];
		UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(selectControl:)];
		tap.enabled = NO;
		tap.cancelsTouchesInView = YES;
		[control addGestureRecognizer:tap];
		[self.editGestures addObject:tap];
	}
}

- (void)updateEditorSelectionLabel
{
	if (self.selectedControl == nil)
	{
		self.editorLabel.text = @"Tap or drag a control • − / + resizes";
		self.editorSmallerButton.enabled = NO;
		self.editorLargerButton.enabled = NO;
		return;
	}
	CGFloat scale = [self.layoutScales[self.selectedControl.accessibilityIdentifier] doubleValue];
	if (!isfinite(scale) || (scale <= 0.0))
		scale = 1.0;
	self.editorLabel.text = [NSString stringWithFormat:@"%@ • Drag to move • Size %.0f%%",
	                                                     self.selectedControl.accessibilityLabel ?: @"Control", scale * 100.0];
	self.editorSmallerButton.enabled = scale > 0.70;
	self.editorLargerButton.enabled = scale < 1.50;
}

- (void)selectControl:(UIGestureRecognizer *)gesture
{
	if (!self.layoutEditing || ![self.editableControls containsObject:gesture.view])
		return;
	self.selectedControl = gesture.view;
	[self updateEditorSelectionLabel];
}

- (void)adjustSelectedControlScaleBy:(CGFloat)delta
{
	UIView *control = self.selectedControl;
	if (!self.layoutEditing || (control == nil))
		return;
	NSString *identifier = control.accessibilityIdentifier;
	CGFloat scale = [self.layoutScales[identifier] doubleValue];
	if (!isfinite(scale) || (scale <= 0.0))
		scale = 1.0;
	scale = MIN(1.50, MAX(0.70, round((scale + delta) * 10.0) / 10.0));
	self.layoutScales[identifier] = @(scale);
	CGRect safe = [self usableBounds];
	if ((CGRectGetWidth(safe) > 0.0) && (CGRectGetHeight(safe) > 0.0))
	{
		self.layoutCenters[identifier] = @[
			@((control.center.x - CGRectGetMinX(safe)) / CGRectGetWidth(safe)),
			@((control.center.y - CGRectGetMinY(safe)) / CGRectGetHeight(safe)),
		];
	}
	[self updateEditorSelectionLabel];
	[self.view setNeedsLayout];
}

- (void)shrinkSelectedControl
{
	[self adjustSelectedControlScaleBy:-0.10];
}

- (void)growSelectedControl
{
	[self adjustSelectedControlScaleBy:0.10];
}

- (void)moveControl:(UIPanGestureRecognizer *)gesture
{
	UIView *control = gesture.view;
	if (!self.layoutEditing || (control == nil))
	{
		return;
	}
	if (gesture.state == UIGestureRecognizerStateBegan)
	{
		[self selectControl:gesture];
	}
	CGPoint translation = [gesture translationInView:self.view];
	control.center = CGPointMake(control.center.x + translation.x, control.center.y + translation.y);
	[gesture setTranslation:CGPointZero inView:self.view];
	[self clampControlToUsableBounds:control];
	CGRect safe = [self usableBounds];
	if ((CGRectGetWidth(safe) > 0.0) && (CGRectGetHeight(safe) > 0.0))
	{
		self.layoutCenters[control.accessibilityIdentifier] = @[
			@((control.center.x - CGRectGetMinX(safe)) / CGRectGetWidth(safe)),
			@((control.center.y - CGRectGetMinY(safe)) / CGRectGetHeight(safe)),
		];
	}
}

- (void)beginLayoutEditing
{
	if (self.layoutEditing || (self.presentedViewController != nil))
	{
		return;
	}
	[self resetControlState];
	self.layoutEditing = YES;
	for (UIView *control in self.editableControls)
	{
		if ([control isKindOfClass:CTRPadInputButton.class])
			((CTRPadInputButton *)control).layoutEditing = YES;
		else if ([control isKindOfClass:CTRPadTouchStickView.class])
			((CTRPadTouchStickView *)control).layoutEditing = YES;
	}
	for (UIGestureRecognizer *gesture in self.editGestures)
		gesture.enabled = YES;
	self.editorPanel.hidden = NO;
	self.selectedControl = nil;
	[self updateEditorSelectionLabel];
	[self.view setNeedsLayout];
	UIAccessibilityPostNotification(UIAccessibilityScreenChangedNotification, self.editorLabel);
}

- (void)saveCurrentLayout
{
	if (self.layoutProfile.length == 0)
		return;
	if ((self.layoutCenters.count == 0) && (self.layoutScales.count == 0))
	{
		[NSUserDefaults.standardUserDefaults removeObjectForKey:[self layoutStorageKey]];
		return;
	}
	[NSUserDefaults.standardUserDefaults setObject:@{
		@"centers" : [self.layoutCenters copy],
		@"scales" : [self.layoutScales copy],
	} forKey:[self layoutStorageKey]];
}

- (void)endLayoutEditing
{
	if (!self.layoutEditing)
		return;
	[self resetControlState];
	[self saveCurrentLayout];
	self.layoutEditing = NO;
	for (UIView *control in self.editableControls)
	{
		if ([control isKindOfClass:CTRPadInputButton.class])
			((CTRPadInputButton *)control).layoutEditing = NO;
		else if ([control isKindOfClass:CTRPadTouchStickView.class])
			((CTRPadTouchStickView *)control).layoutEditing = NO;
	}
	for (UIGestureRecognizer *gesture in self.editGestures)
		gesture.enabled = NO;
	self.selectedControl = nil;
	self.editorPanel.hidden = YES;
	[self.view setNeedsLayout];
	UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, @"Touch layout saved.");
}

- (void)resetCurrentLayout
{
	if (self.layoutProfile.length == 0)
		self.layoutProfile = [self currentLayoutProfile];
	[self.layoutCenters removeAllObjects];
	[self.layoutScales removeAllObjects];
	self.selectedControl = nil;
	[NSUserDefaults.standardUserDefaults removeObjectForKey:[self layoutStorageKey]];
	[self updateEditorSelectionLabel];
	[self.view setNeedsLayout];
	UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, @"This touch layout was reset.");
}

- (void)resetAllLayouts
{
	for (NSString *profile in @[ @"phone-left-v1", @"phone-right-v1", @"tablet-left-v1", @"tablet-right-v1" ])
	{
		NSString *key = [NSString stringWithFormat:@"%@.%@", s_touchLayoutKeyPrefix, profile];
		[NSUserDefaults.standardUserDefaults removeObjectForKey:key];
	}
	[self.layoutCenters removeAllObjects];
	[self.layoutScales removeAllObjects];
	self.selectedControl = nil;
	self.layoutProfile = nil;
	[self.view setNeedsLayout];
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
	Platform_InputTouchSetEnabled(controller.touchControlsEnabled ? 1 : 0);
	return 1;
}

void NativeIOSTouch_End(void)
{
	if (!NSThread.isMainThread)
	{
		return;
	}
	[s_touchOverlayController resetControlState];
	Platform_InputTouchSetEnabled(0);
	s_discReselectionCallback = NULL;
	s_discReselectionUserdata = NULL;
	[s_touchOverlayController willMoveToParentViewController:nil];
	[s_touchOverlayController.view removeFromSuperview];
	[s_touchOverlayController removeFromParentViewController];
	s_touchOverlayController = nil;
}

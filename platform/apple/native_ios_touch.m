#import <UIKit/UIKit.h>

#include <math.h>

#include "platform/native_input.h"
#include "platform/native_ios_touch.h"

@interface CTRPadTouchPassthroughView : UIView
@end

@interface CTRPadTouchStickView : UIView
@property(nonatomic, strong) UIView *knob;
@property(nonatomic, strong) UILabel *label;
@property(nonatomic, assign) BOOL trackingTouch;
@property(nonatomic, assign) unsigned int directionMask;
@end

@interface CTRPadTouchOverlayViewController : UIViewController
@end

static CTRPadTouchOverlayViewController *s_touchOverlayController;

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
	button.backgroundColor = [color colorWithAlphaComponent:0.58];
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

- (void)viewDidLoad
{
	[super viewDidLoad];

	CTRPadTouchStickView *stick = [[CTRPadTouchStickView alloc] init];
	UIButton *cross = [self buttonWithTitle:@"GAS\n✕" mask:PLATFORM_INPUT_TOUCH_CROSS color:[UIColor colorWithRed:0.08 green:0.42 blue:0.95 alpha:1.0]];
	UIButton *square = [self buttonWithTitle:@"BRAKE\n□" mask:PLATFORM_INPUT_TOUCH_SQUARE color:[UIColor colorWithRed:0.93 green:0.19 blue:0.52 alpha:1.0]];
	UIButton *circle = [self buttonWithTitle:@"ITEM\n○" mask:PLATFORM_INPUT_TOUCH_CIRCLE color:[UIColor colorWithRed:0.93 green:0.16 blue:0.18 alpha:1.0]];
	UIButton *triangle = [self buttonWithTitle:@"VIEW\n△" mask:PLATFORM_INPUT_TOUCH_TRIANGLE color:[UIColor colorWithRed:0.08 green:0.68 blue:0.32 alpha:1.0]];
	UIButton *leftDrift = [self buttonWithTitle:@"L DRIFT / BOOST" mask:PLATFORM_INPUT_TOUCH_L1 color:[UIColor colorWithRed:0.94 green:0.59 blue:0.10 alpha:1.0]];
	UIButton *rightDrift = [self buttonWithTitle:@"R DRIFT / BOOST" mask:PLATFORM_INPUT_TOUCH_R1 color:[UIColor colorWithRed:0.94 green:0.59 blue:0.10 alpha:1.0]];
	UIButton *start = [self buttonWithTitle:@"PAUSE" mask:PLATFORM_INPUT_TOUCH_START color:[UIColor colorWithWhite:0.08 alpha:1.0]];
	UIButton *select = [self buttonWithTitle:@"SELECT" mask:PLATFORM_INPUT_TOUCH_SELECT color:[UIColor colorWithWhite:0.08 alpha:1.0]];

	cross.accessibilityIdentifier = @"ctrpad.touch.cross";
	square.accessibilityIdentifier = @"ctrpad.touch.square";
	circle.accessibilityIdentifier = @"ctrpad.touch.circle";
	triangle.accessibilityIdentifier = @"ctrpad.touch.triangle";
	leftDrift.accessibilityIdentifier = @"ctrpad.touch.l1";
	rightDrift.accessibilityIdentifier = @"ctrpad.touch.r1";
	start.accessibilityIdentifier = @"ctrpad.touch.start";
	select.accessibilityIdentifier = @"ctrpad.touch.select";

	NSArray<UIView *> *controls = @[ stick, cross, square, circle, triangle, leftDrift, rightDrift, start, select ];
	for (UIView *control in controls)
	{
		[self.view addSubview:control];
	}

	UILayoutGuide *safe = self.view.safeAreaLayoutGuide;
	[NSLayoutConstraint activateConstraints:@[
		[stick.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:24.0],
		[stick.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor constant:-24.0],
		[stick.widthAnchor constraintEqualToConstant:174.0],
		[stick.heightAnchor constraintEqualToAnchor:stick.widthAnchor],

		[leftDrift.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:18.0],
		[leftDrift.topAnchor constraintEqualToAnchor:safe.topAnchor constant:10.0],
		[leftDrift.widthAnchor constraintEqualToConstant:144.0],
		[leftDrift.heightAnchor constraintEqualToConstant:54.0],

		[rightDrift.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-18.0],
		[rightDrift.topAnchor constraintEqualToAnchor:safe.topAnchor constant:10.0],
		[rightDrift.widthAnchor constraintEqualToConstant:144.0],
		[rightDrift.heightAnchor constraintEqualToConstant:54.0],

		[start.topAnchor constraintEqualToAnchor:safe.topAnchor constant:12.0],
		[start.centerXAnchor constraintEqualToAnchor:safe.centerXAnchor constant:48.0],
		[start.widthAnchor constraintEqualToConstant:82.0],
		[start.heightAnchor constraintEqualToConstant:40.0],
		[select.topAnchor constraintEqualToAnchor:safe.topAnchor constant:12.0],
		[select.centerXAnchor constraintEqualToAnchor:safe.centerXAnchor constant:-48.0],
		[select.widthAnchor constraintEqualToConstant:82.0],
		[select.heightAnchor constraintEqualToConstant:40.0],

		[cross.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-24.0],
		[cross.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor constant:-22.0],
		[cross.widthAnchor constraintEqualToConstant:92.0],
		[cross.heightAnchor constraintEqualToAnchor:cross.widthAnchor],

		[square.trailingAnchor constraintEqualToAnchor:cross.leadingAnchor constant:-18.0],
		[square.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor constant:-34.0],
		[square.widthAnchor constraintEqualToConstant:64.0],
		[square.heightAnchor constraintEqualToAnchor:square.widthAnchor],

		[circle.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-36.0],
		[circle.bottomAnchor constraintEqualToAnchor:cross.topAnchor constant:-16.0],
		[circle.widthAnchor constraintEqualToConstant:64.0],
		[circle.heightAnchor constraintEqualToAnchor:circle.widthAnchor],

		[triangle.trailingAnchor constraintEqualToAnchor:circle.leadingAnchor constant:-14.0],
		[triangle.centerYAnchor constraintEqualToAnchor:circle.centerYAnchor],
		[triangle.widthAnchor constraintEqualToConstant:64.0],
		[triangle.heightAnchor constraintEqualToAnchor:triangle.widthAnchor],
	]];

	for (UIButton *button in @[ cross, square, circle, triangle ])
	{
		button.layer.cornerRadius = button == cross ? 46.0 : 32.0;
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

int NativeIOSTouch_Begin(void)
{
	if (!NSThread.isMainThread)
	{
		return 0;
	}
	if (s_touchOverlayController != nil)
	{
		return 1;
	}

	UIWindow *gameWindow = CTRPadTouch_FindGameWindow();
	if (gameWindow == nil)
	{
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
	[s_touchOverlayController willMoveToParentViewController:nil];
	[s_touchOverlayController.view removeFromSuperview];
	[s_touchOverlayController removeFromParentViewController];
	s_touchOverlayController = nil;
}

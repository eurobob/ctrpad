#import <AppKit/AppKit.h>

#include <SDL3/SDL.h>

#include "platform/native_input.h"
#include "platform/native_macos_controls.h"
#include "platform/native_renderer.h"

static NSString *const s_keyboardPreferencePrefix = @"CTRPad.Keyboard";
static NSString *const s_internalResolutionScaleKey = @"CTRPadInternalResolutionScale";
static const NSInteger s_defaultMacInternalResolutionScale = 4;

@interface CTRPadControlsController : NSObject <NSWindowDelegate>
@property(nonatomic, weak) NSWindow *gameWindow;
@property(nonatomic, strong) NSButton *optionsButton;
@property(nonatomic, strong) NSPanel *panel;
@property(nonatomic, strong) NSMutableArray<NSButton *> *bindingButtons;
@property(nonatomic, strong) NSButton *capturingButton;
@property(nonatomic, strong) NSSegmentedControl *resolutionControl;
@property(nonatomic, strong) NSTextField *resolutionStatusLabel;
@property(nonatomic, strong) NSTextField *controllerStatusLabel;
@property(nonatomic, strong) id eventMonitor;
@end

@implementation CTRPadControlsController

- (instancetype)initWithGameWindow:(NSWindow *)gameWindow
{
	self = [super init];
	if (self == nil)
	{
		return nil;
	}

	_gameWindow = gameWindow;
	_bindingButtons = [NSMutableArray array];
	[self loadBindings];
	[self loadDisplayPreferences];
	[self installOptionsButton];
	return self;
}

- (void)loadDisplayPreferences
{
	NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
	NSInteger requestedScale = s_defaultMacInternalResolutionScale;
	if ([defaults objectForKey:s_internalResolutionScaleKey] != nil)
	{
		requestedScale = [defaults integerForKey:s_internalResolutionScaleKey] + 1;
	}
	NSInteger appliedScale = NativeRenderer_SetInternalResolutionScale((int)requestedScale);
	if (appliedScale != requestedScale)
	{
		[defaults setInteger:appliedScale - 1 forKey:s_internalResolutionScaleKey];
	}
}

- (NSString *)preferenceKeyForAction:(NSInteger)action binding:(NSInteger)binding
{
	return [NSString stringWithFormat:@"%@.%ld.%ld", s_keyboardPreferencePrefix, (long)action, (long)binding];
}

- (void)loadBindings
{
	NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
	Platform_InputResetKeyboardBindings();
	for (NSInteger action = 0; action < PLATFORM_INPUT_KEYBOARD_ACTION_COUNT; action++)
	{
		for (NSInteger binding = 0; binding < PLATFORM_INPUT_KEYBOARD_BINDING_COUNT; binding++)
		{
			NSString *key = [self preferenceKeyForAction:action binding:binding];
			if ([defaults objectForKey:key] != nil)
			{
				Platform_InputSetKeyboardBinding((int)action, (int)binding, (int)[defaults integerForKey:key]);
			}
		}
	}
}

- (void)installOptionsButton
{
	NSView *contentView = self.gameWindow.contentView;
	if (contentView == nil)
	{
		return;
	}

	NSButton *button = [NSButton buttonWithTitle:@"•••" target:self action:@selector(showControls:)];
	button.bezelStyle = NSBezelStyleTexturedRounded;
	button.font = [NSFont boldSystemFontOfSize:17.0];
	button.toolTip = @"Display and controls";
	button.frame = NSMakeRect(NSWidth(contentView.bounds) - 58.0, NSHeight(contentView.bounds) - 44.0, 46.0, 30.0);
	button.autoresizingMask = NSViewMinXMargin | NSViewMinYMargin;
	[button setAccessibilityLabel:@"Options"];
	[contentView addSubview:button positioned:NSWindowAbove relativeTo:nil];
	self.optionsButton = button;
}

- (NSTextField *)labelWithString:(NSString *)text frame:(NSRect)frame bold:(BOOL)bold
{
	NSTextField *label = [NSTextField labelWithString:text];
	label.frame = frame;
	label.font = bold ? [NSFont boldSystemFontOfSize:13.0] : [NSFont systemFontOfSize:13.0];
	label.lineBreakMode = NSLineBreakByTruncatingTail;
	return label;
}

- (NSTextField *)multilineLabelWithString:(NSString *)text frame:(NSRect)frame
{
	NSTextField *label = [NSTextField wrappingLabelWithString:text];
	label.frame = frame;
	label.font = [NSFont systemFontOfSize:13.0];
	label.textColor = NSColor.secondaryLabelColor;
	label.maximumNumberOfLines = 0;
	return label;
}

- (NSString *)displayNameForScancode:(int)scancode
{
	if (scancode == SDL_SCANCODE_UNKNOWN)
	{
		return @"Not set";
	}
	const char *name = SDL_GetScancodeName((SDL_Scancode)scancode);
	if ((name == NULL) || (name[0] == '\0'))
	{
		return [NSString stringWithFormat:@"Key %d", scancode];
	}
	return [NSString stringWithUTF8String:name];
}

- (void)addDisplayTabToTabView:(NSTabView *)tabView
{
	NSTabViewItem *item = [[NSTabViewItem alloc] initWithIdentifier:@"display"];
	item.label = @"Display";
	NSView *view = [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 660.0, 515.0)];

	[view addSubview:[self labelWithString:@"Internal resolution" frame:NSMakeRect(24.0, 466.0, 260.0, 24.0) bold:YES]];
	[view addSubview:[self multilineLabelWithString:@"Render game geometry at a higher internal resolution, then scale it cleanly to the window. This smooths polygon edges while preserving the original texture detail."
	                                            frame:NSMakeRect(24.0, 416.0, 612.0, 42.0)]];

	NSSegmentedControl *resolution = [NSSegmentedControl segmentedControlWithLabels:@[ @"1×", @"2×", @"3×", @"4×" ]
	                                                                              trackingMode:NSSegmentSwitchTrackingSelectOne
	                                                                                    target:self
	                                                                                    action:@selector(resolutionChanged:)];
	resolution.frame = NSMakeRect(24.0, 355.0, 420.0, 38.0);
	resolution.segmentStyle = NSSegmentStyleRounded;
	[resolution setAccessibilityLabel:@"Internal resolution"];
	NSInteger requestedScale = s_defaultMacInternalResolutionScale;
	if ([NSUserDefaults.standardUserDefaults objectForKey:s_internalResolutionScaleKey] != nil)
	{
		requestedScale = [NSUserDefaults.standardUserDefaults integerForKey:s_internalResolutionScaleKey] + 1;
	}
	NSInteger appliedScale = NativeRenderer_SetInternalResolutionScale((int)requestedScale);
	resolution.selectedSegment = appliedScale - 1;
	[view addSubview:resolution];
	self.resolutionControl = resolution;

	NSTextField *status = [self labelWithString:@"" frame:NSMakeRect(24.0, 321.0, 612.0, 22.0) bold:NO];
	status.textColor = NSColor.secondaryLabelColor;
	[view addSubview:status];
	self.resolutionStatusLabel = status;
	[self updateResolutionStatus];

	[view addSubview:[self labelWithString:@"Recommended on Apple Silicon" frame:NSMakeRect(24.0, 267.0, 300.0, 24.0) bold:YES]];
	[view addSubview:[self multilineLabelWithString:@"Start with 4× for the cleanest geometry. Choose a lower setting if you prefer reduced GPU use. Changes apply immediately and are remembered for the next launch."
	                                            frame:NSMakeRect(24.0, 217.0, 612.0, 42.0)]];
	[view addSubview:[self labelWithString:@"Window and fullscreen" frame:NSMakeRect(24.0, 164.0, 300.0, 24.0) bold:YES]];
	[view addSubview:[self multilineLabelWithString:@"Resize the window freely; CTRPad preserves the game's 4:3 presentation. Press F11 to enter or leave fullscreen."
	                                            frame:NSMakeRect(24.0, 124.0, 612.0, 36.0)]];

	NSButton *reset = [NSButton buttonWithTitle:@"Restore Display Default" target:self action:@selector(resetDisplay:)];
	reset.bezelStyle = NSBezelStyleRounded;
	reset.frame = NSMakeRect(24.0, 30.0, 190.0, 32.0);
	[view addSubview:reset];

	item.view = view;
	[tabView addTabViewItem:item];
}

- (void)addKeyboardTabToTabView:(NSTabView *)tabView
{
	static NSString *const actionNames[PLATFORM_INPUT_KEYBOARD_ACTION_COUNT] = {
		@"Steer / menu up", @"Steer / menu down", @"Steer / menu left", @"Steer / menu right",
		@"Accelerate / confirm (×)", @"Brake / reverse (□)", @"Use item (○)", @"Camera / back (△)",
		@"Drift left (L1)", @"Drift right (R1)", @"Left trigger (L2)", @"Right trigger (R2)",
		@"Left stick click (L3)", @"Right stick click (R3)", @"Start / pause", @"Select",
	};
	NSTabViewItem *item = [[NSTabViewItem alloc] initWithIdentifier:@"keyboard"];
	item.label = @"Keyboard";
	NSView *view = [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 660.0, 515.0)];

	[view addSubview:[self labelWithString:@"Keyboard layout" frame:NSMakeRect(18.0, 481.0, 240.0, 22.0) bold:YES]];
	NSTextField *help = [self labelWithString:@"Select a binding, then press a key. Backspace clears it. Every change is saved automatically."
	                                         frame:NSMakeRect(18.0, 456.0, 624.0, 20.0) bold:NO];
	help.textColor = NSColor.secondaryLabelColor;
	[view addSubview:help];
	[view addSubview:[self labelWithString:@"Action" frame:NSMakeRect(18.0, 430.0, 220.0, 20.0) bold:YES]];
	[view addSubview:[self labelWithString:@"Primary" frame:NSMakeRect(244.0, 430.0, 180.0, 20.0) bold:YES]];
	[view addSubview:[self labelWithString:@"Alternate" frame:NSMakeRect(438.0, 430.0, 180.0, 20.0) bold:YES]];

	for (NSInteger action = 0; action < PLATFORM_INPUT_KEYBOARD_ACTION_COUNT; action++)
	{
		CGFloat y = 402.0 - ((CGFloat)action * 24.0);
		[view addSubview:[self labelWithString:actionNames[action] frame:NSMakeRect(18.0, y + 2.0, 220.0, 20.0) bold:NO]];
		for (NSInteger binding = 0; binding < PLATFORM_INPUT_KEYBOARD_BINDING_COUNT; binding++)
		{
			int scancode = Platform_InputGetKeyboardBinding((int)action, (int)binding);
			NSString *bindingName = binding == 0 ? @"Primary" : @"Alternate";
			NSButton *button = [NSButton buttonWithTitle:[self displayNameForScancode:scancode]
			                                      target:self
			                                      action:@selector(beginCapture:)];
			button.bezelStyle = NSBezelStyleRounded;
			button.tag = action * PLATFORM_INPUT_KEYBOARD_BINDING_COUNT + binding;
			button.frame = NSMakeRect(binding == 0 ? 244.0 : 438.0, y, 180.0, 22.0);
			[button setAccessibilityLabel:[NSString stringWithFormat:@"%@, %@ binding", actionNames[action], bindingName]];
			[button setAccessibilityValue:button.title];
			[view addSubview:button];
			[self.bindingButtons addObject:button];
		}
	}

	NSButton *reset = [NSButton buttonWithTitle:@"Restore Keyboard Defaults" target:self action:@selector(resetBindings:)];
	reset.bezelStyle = NSBezelStyleRounded;
	reset.frame = NSMakeRect(18.0, 10.0, 202.0, 30.0);
	[view addSubview:reset];

	item.view = view;
	[tabView addTabViewItem:item];
}

- (void)addControllerTabToTabView:(NSTabView *)tabView
{
	static NSString *const actions[] = {
		@"Steer and navigate", @"Accelerate / confirm (×)", @"Brake / reverse (□)", @"Use item (○)",
		@"Camera / back (△)", @"Drift / boost", @"Triggers", @"Stick clicks", @"Start / pause", @"Select",
	};
	static NSString *const inputs[] = {
		@"Left stick or D-pad", @"South / Cross", @"West / Square", @"East / Circle",
		@"North / Triangle", @"Left and right shoulders", @"Left and right triggers", @"L3 and R3", @"Start / Menu", @"Back / Share",
	};
	NSTabViewItem *item = [[NSTabViewItem alloc] initWithIdentifier:@"controller"];
	item.label = @"Controller";
	NSView *view = [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 660.0, 515.0)];

	[view addSubview:[self labelWithString:@"Standard controller layout" frame:NSMakeRect(24.0, 470.0, 300.0, 24.0) bold:YES]];
	[view addSubview:[self multilineLabelWithString:@"CTRPad uses the same SDL gamepad mapping on Mac, iPhone, and iPad. Connect a controller before or during play; hot-plugging is supported."
	                                            frame:NSMakeRect(24.0, 426.0, 612.0, 38.0)]];
	NSTextField *status = [self labelWithString:@"" frame:NSMakeRect(24.0, 392.0, 612.0, 22.0) bold:YES];
	[view addSubview:status];
	self.controllerStatusLabel = status;
	[self updateControllerStatus];

	[view addSubview:[self labelWithString:@"Game action" frame:NSMakeRect(24.0, 352.0, 280.0, 20.0) bold:YES]];
	[view addSubview:[self labelWithString:@"Controller input" frame:NSMakeRect(330.0, 352.0, 280.0, 20.0) bold:YES]];
	for (NSInteger row = 0; row < 10; row++)
	{
		CGFloat y = 322.0 - ((CGFloat)row * 29.0);
		[view addSubview:[self labelWithString:actions[row] frame:NSMakeRect(24.0, y, 280.0, 20.0) bold:NO]];
		[view addSubview:[self labelWithString:inputs[row] frame:NSMakeRect(330.0, y, 280.0, 20.0) bold:NO]];
	}

	item.view = view;
	[tabView addTabViewItem:item];
}

- (void)buildPanel
{
	const CGFloat panelWidth = 720.0;
	const CGFloat panelHeight = 720.0;
	NSPanel *panel = [[NSPanel alloc]
	    initWithContentRect:NSMakeRect(0.0, 0.0, panelWidth, panelHeight)
	              styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskUtilityWindow
	                backing:NSBackingStoreBuffered
	                  defer:NO];
	panel.title = @"CTRPad Options";
	panel.delegate = self;
	panel.releasedWhenClosed = NO;
	panel.floatingPanel = YES;
	NSView *content = panel.contentView;

	NSTextField *title = [self labelWithString:@"Options" frame:NSMakeRect(24.0, 669.0, 300.0, 30.0) bold:YES];
	title.font = [NSFont boldSystemFontOfSize:24.0];
	[content addSubview:title];
	NSTextField *subtitle = [self labelWithString:@"Tune the Mac presentation and choose how you play."
	                                          frame:NSMakeRect(24.0, 642.0, 672.0, 22.0) bold:NO];
	subtitle.textColor = NSColor.secondaryLabelColor;
	[content addSubview:subtitle];

	NSTabView *tabs = [[NSTabView alloc] initWithFrame:NSMakeRect(20.0, 68.0, 680.0, 558.0)];
	[self addDisplayTabToTabView:tabs];
	[self addKeyboardTabToTabView:tabs];
	[self addControllerTabToTabView:tabs];
	[content addSubview:tabs];

	NSButton *done = [NSButton buttonWithTitle:@"Done" target:self action:@selector(closeControls:)];
	done.bezelStyle = NSBezelStyleRounded;
	done.keyEquivalent = @"\r";
	done.frame = NSMakeRect(596.0, 20.0, 100.0, 32.0);
	[content addSubview:done];

	self.panel = panel;
}

- (void)showControls:(id)sender
{
	(void)sender;
	if (self.panel == nil)
	{
		[self buildPanel];
	}
	[self updateControllerStatus];
	Platform_InputSetKeyboardSuppressed(1);
	[self.gameWindow addChildWindow:self.panel ordered:NSWindowAbove];
	[self.panel center];
	[self.panel makeKeyAndOrderFront:nil];
}

- (void)updateResolutionStatus
{
	NSInteger scale = self.resolutionControl.selectedSegment + 1;
	self.resolutionStatusLabel.stringValue = [NSString stringWithFormat:@"Currently rendering game geometry at %ld× internal resolution.", (long)scale];
	[self.resolutionStatusLabel setAccessibilityLabel:@"Internal resolution status"];
	[self.resolutionStatusLabel setAccessibilityValue:self.resolutionStatusLabel.stringValue];
}

- (void)resolutionChanged:(NSSegmentedControl *)sender
{
	NSInteger requestedScale = sender.selectedSegment + 1;
	NSInteger appliedScale = NativeRenderer_SetInternalResolutionScale((int)requestedScale);
	sender.selectedSegment = appliedScale - 1;
	[NSUserDefaults.standardUserDefaults setInteger:appliedScale - 1 forKey:s_internalResolutionScaleKey];
	[self updateResolutionStatus];
}

- (void)resetDisplay:(id)sender
{
	(void)sender;
	[NSUserDefaults.standardUserDefaults removeObjectForKey:s_internalResolutionScaleKey];
	NSInteger appliedScale = NativeRenderer_SetInternalResolutionScale((int)s_defaultMacInternalResolutionScale);
	self.resolutionControl.selectedSegment = appliedScale - 1;
	[self updateResolutionStatus];
}

- (void)updateControllerStatus
{
	if (self.controllerStatusLabel == nil)
	{
		return;
	}
	int count = 0;
	SDL_JoystickID *gamepads = SDL_GetGamepads(&count);
	if ((gamepads == NULL) || (count <= 0))
	{
		self.controllerStatusLabel.stringValue = @"No controller detected — keyboard play remains available.";
		self.controllerStatusLabel.textColor = NSColor.secondaryLabelColor;
	}
	else
	{
		NSMutableArray<NSString *> *names = [NSMutableArray arrayWithCapacity:(NSUInteger)count];
		for (int index = 0; index < count; index++)
		{
			const char *name = SDL_GetGamepadNameForID(gamepads[index]);
			[names addObject:(name != NULL) ? [NSString stringWithUTF8String:name] : @"Controller"];
		}
		self.controllerStatusLabel.stringValue = [NSString stringWithFormat:@"Connected: %@", [names componentsJoinedByString:@", "]];
		self.controllerStatusLabel.textColor = NSColor.labelColor;
	}
	SDL_free(gamepads);
	[self.controllerStatusLabel setAccessibilityLabel:@"Controller connection status"];
	[self.controllerStatusLabel setAccessibilityValue:self.controllerStatusLabel.stringValue];
}

- (void)closeControls:(id)sender
{
	(void)sender;
	[self.panel close];
}

- (void)windowWillClose:(NSNotification *)notification
{
	if (notification.object != self.panel)
	{
		return;
	}
	self.capturingButton = nil;
	Platform_InputSetKeyboardSuppressed(0);
	if (self.panel.parentWindow != nil)
	{
		[self.panel.parentWindow removeChildWindow:self.panel];
	}
	[self.gameWindow makeKeyAndOrderFront:nil];
}

- (void)refreshBindingButtons
{
	for (NSButton *button in self.bindingButtons)
	{
		NSInteger action = button.tag / PLATFORM_INPUT_KEYBOARD_BINDING_COUNT;
		NSInteger binding = button.tag % PLATFORM_INPUT_KEYBOARD_BINDING_COUNT;
		button.title = [self displayNameForScancode:Platform_InputGetKeyboardBinding((int)action, (int)binding)];
		[button setAccessibilityValue:button.title];
	}
}

- (void)resetBindings:(id)sender
{
	(void)sender;
	NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
	for (NSInteger action = 0; action < PLATFORM_INPUT_KEYBOARD_ACTION_COUNT; action++)
	{
		for (NSInteger binding = 0; binding < PLATFORM_INPUT_KEYBOARD_BINDING_COUNT; binding++)
		{
			[defaults removeObjectForKey:[self preferenceKeyForAction:action binding:binding]];
		}
	}
	Platform_InputResetKeyboardBindings();
	self.capturingButton = nil;
	[self refreshBindingButtons];
}

- (void)beginCapture:(NSButton *)sender
{
	if ((self.capturingButton != nil) && (self.capturingButton != sender))
	{
		[self refreshBindingButtons];
	}
	self.capturingButton = sender;
	sender.title = @"Press a key…";

	if (self.eventMonitor == nil)
	{
		__weak CTRPadControlsController *weakSelf = self;
		self.eventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown | NSEventMaskFlagsChanged
		                                                       handler:^NSEvent *(NSEvent *event) {
			CTRPadControlsController *strongSelf = weakSelf;
			if ((strongSelf == nil) || (strongSelf.capturingButton == nil))
			{
				return event;
			}
			int scancode = [strongSelf scancodeForEvent:event];
			if (scancode < SDL_SCANCODE_UNKNOWN)
			{
				return nil;
			}
			[strongSelf finishCaptureWithScancode:scancode];
			return nil;
		}];
	}
}

- (void)finishCaptureWithScancode:(int)scancode
{
	NSButton *button = self.capturingButton;
	if (button == nil)
	{
		return;
	}
	NSInteger action = button.tag / PLATFORM_INPUT_KEYBOARD_BINDING_COUNT;
	NSInteger binding = button.tag % PLATFORM_INPUT_KEYBOARD_BINDING_COUNT;
	if (Platform_InputSetKeyboardBinding((int)action, (int)binding, scancode))
	{
		[NSUserDefaults.standardUserDefaults setInteger:scancode
		                                       forKey:[self preferenceKeyForAction:action binding:binding]];
	}
	self.capturingButton = nil;
	[self refreshBindingButtons];
}

- (int)scancodeForEvent:(NSEvent *)event
{
	if (event.type == NSEventTypeFlagsChanged)
	{
		NSEventModifierFlags flags = event.modifierFlags & NSEventModifierFlagDeviceIndependentFlagsMask;
		switch (event.keyCode)
		{
		case 56:
			return (flags & NSEventModifierFlagShift) != 0 ? SDL_SCANCODE_LSHIFT : -1;
		case 60:
			return (flags & NSEventModifierFlagShift) != 0 ? SDL_SCANCODE_RSHIFT : -1;
		case 59:
			return (flags & NSEventModifierFlagControl) != 0 ? SDL_SCANCODE_LCTRL : -1;
		case 62:
			return (flags & NSEventModifierFlagControl) != 0 ? SDL_SCANCODE_RCTRL : -1;
		case 58:
			return (flags & NSEventModifierFlagOption) != 0 ? SDL_SCANCODE_LALT : -1;
		case 61:
			return (flags & NSEventModifierFlagOption) != 0 ? SDL_SCANCODE_RALT : -1;
		default:
			return -1;
		}
	}

	// Backspace deliberately means “clear” while a binding button is waiting.
	if (event.keyCode == 51)
	{
		return SDL_SCANCODE_UNKNOWN;
	}
	switch (event.keyCode)
	{
	case 36: return SDL_SCANCODE_RETURN;
	case 48: return SDL_SCANCODE_TAB;
	case 49: return SDL_SCANCODE_SPACE;
	case 53: return SDL_SCANCODE_ESCAPE;
	case 117: return SDL_SCANCODE_DELETE;
	case 123: return SDL_SCANCODE_LEFT;
	case 124: return SDL_SCANCODE_RIGHT;
	case 125: return SDL_SCANCODE_DOWN;
	case 126: return SDL_SCANCODE_UP;
	case 115: return SDL_SCANCODE_HOME;
	case 119: return SDL_SCANCODE_END;
	case 116: return SDL_SCANCODE_PAGEUP;
	case 121: return SDL_SCANCODE_PAGEDOWN;
	case 122: return SDL_SCANCODE_F1;
	case 120: return SDL_SCANCODE_F2;
	case 99: return SDL_SCANCODE_F3;
	case 118: return SDL_SCANCODE_F4;
	case 96: return SDL_SCANCODE_F5;
	case 97: return SDL_SCANCODE_F6;
	case 98: return SDL_SCANCODE_F7;
	case 100: return SDL_SCANCODE_F8;
	case 101: return SDL_SCANCODE_F9;
	case 109: return SDL_SCANCODE_F10;
	case 103: return SDL_SCANCODE_F11;
	case 111: return SDL_SCANCODE_F12;
	default:
		break;
	}

	NSString *characters = event.charactersIgnoringModifiers.lowercaseString;
	if (characters.length == 0)
	{
		return -1;
	}
	unichar character = [characters characterAtIndex:0];
	if (character > 0x7f)
	{
		return -1;
	}
	SDL_Scancode scancode = SDL_GetScancodeFromKey((SDL_Keycode)character, NULL);
	return scancode != SDL_SCANCODE_UNKNOWN ? (int)scancode : -1;
}

- (void)shutdown
{
	Platform_InputSetKeyboardSuppressed(0);
	if (self.eventMonitor != nil)
	{
		[NSEvent removeMonitor:self.eventMonitor];
		self.eventMonitor = nil;
	}
	[self.panel orderOut:nil];
	[self.optionsButton removeFromSuperview];
	self.panel = nil;
	self.optionsButton = nil;
}

@end

static CTRPadControlsController *s_controlsController;

void NativeMacOSControls_Install(void *sdlWindow)
{
	@autoreleasepool
	{
		SDL_Window *window = (SDL_Window *)sdlWindow;
		if ((window == NULL) || (s_controlsController != nil))
		{
			return;
		}
		void *cocoaWindow = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
		if (cocoaWindow == NULL)
		{
			return;
		}
		s_controlsController = [[CTRPadControlsController alloc] initWithGameWindow:(__bridge NSWindow *)cocoaWindow];
	}
}

void NativeMacOSControls_Shutdown(void)
{
	@autoreleasepool
	{
		[s_controlsController shutdown];
		s_controlsController = nil;
	}
}

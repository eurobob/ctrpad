#import <AppKit/AppKit.h>

#include <SDL3/SDL.h>

#include "platform/native_input.h"
#include "platform/native_macos_controls.h"

static NSString *const s_keyboardPreferencePrefix = @"CTRPad.Keyboard";

@interface CTRPadControlsController : NSObject <NSWindowDelegate>
@property(nonatomic, weak) NSWindow *gameWindow;
@property(nonatomic, strong) NSButton *optionsButton;
@property(nonatomic, strong) NSPanel *panel;
@property(nonatomic, strong) NSMutableArray<NSButton *> *bindingButtons;
@property(nonatomic, strong) NSButton *capturingButton;
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
	[self installOptionsButton];
	return self;
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
	button.toolTip = @"Keyboard controls";
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

- (void)buildPanel
{
	static NSString *const actionNames[PLATFORM_INPUT_KEYBOARD_ACTION_COUNT] = {
		@"Steer / menu up", @"Steer / menu down", @"Steer / menu left", @"Steer / menu right",
		@"Accelerate / confirm (×)", @"Brake / reverse (□)", @"Use item (○)", @"Camera / back (△)",
		@"Drift left (L1)", @"Drift right (R1)", @"Left trigger (L2)", @"Right trigger (R2)",
		@"Left stick click (L3)", @"Right stick click (R3)", @"Start / pause", @"Select",
	};
	const CGFloat panelWidth = 640.0;
	const CGFloat panelHeight = 650.0;
	NSPanel *panel = [[NSPanel alloc]
	    initWithContentRect:NSMakeRect(0.0, 0.0, panelWidth, panelHeight)
	              styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskUtilityWindow
	                backing:NSBackingStoreBuffered
	                  defer:NO];
	panel.title = @"CTRPad Controls";
	panel.delegate = self;
	panel.releasedWhenClosed = NO;
	panel.floatingPanel = YES;
	NSView *content = panel.contentView;

	[content addSubview:[self labelWithString:@"Keyboard controls" frame:NSMakeRect(24.0, 608.0, 300.0, 24.0) bold:YES]];
	NSTextField *help = [self labelWithString:@"Click a binding, then press a key. Backspace clears it. Changes are saved automatically."
	                                         frame:NSMakeRect(24.0, 581.0, 592.0, 20.0) bold:NO];
	help.textColor = NSColor.secondaryLabelColor;
	[content addSubview:help];
	NSTextField *controller = [self labelWithString:@"Controllers use the same standard SDL gamepad mapping as iPhone and iPad."
	                                               frame:NSMakeRect(24.0, 558.0, 592.0, 20.0) bold:NO];
	controller.textColor = NSColor.secondaryLabelColor;
	[content addSubview:controller];
	[content addSubview:[self labelWithString:@"Action" frame:NSMakeRect(24.0, 530.0, 220.0, 20.0) bold:YES]];
	[content addSubview:[self labelWithString:@"Primary" frame:NSMakeRect(250.0, 530.0, 160.0, 20.0) bold:YES]];
	[content addSubview:[self labelWithString:@"Alternate" frame:NSMakeRect(426.0, 530.0, 160.0, 20.0) bold:YES]];

	for (NSInteger action = 0; action < PLATFORM_INPUT_KEYBOARD_ACTION_COUNT; action++)
	{
		CGFloat y = 500.0 - ((CGFloat)action * 29.0);
		[content addSubview:[self labelWithString:actionNames[action] frame:NSMakeRect(24.0, y + 4.0, 220.0, 20.0) bold:NO]];
		for (NSInteger binding = 0; binding < PLATFORM_INPUT_KEYBOARD_BINDING_COUNT; binding++)
		{
			int scancode = Platform_InputGetKeyboardBinding((int)action, (int)binding);
			NSButton *button = [NSButton buttonWithTitle:[self displayNameForScancode:scancode]
			                                      target:self
			                                      action:@selector(beginCapture:)];
			button.bezelStyle = NSBezelStyleRounded;
			button.tag = action * PLATFORM_INPUT_KEYBOARD_BINDING_COUNT + binding;
			button.frame = NSMakeRect(binding == 0 ? 250.0 : 426.0, y, 164.0, 26.0);
			[content addSubview:button];
			[self.bindingButtons addObject:button];
		}
	}

	NSButton *reset = [NSButton buttonWithTitle:@"Restore Defaults" target:self action:@selector(resetBindings:)];
	reset.bezelStyle = NSBezelStyleRounded;
	reset.frame = NSMakeRect(24.0, 18.0, 140.0, 30.0);
	[content addSubview:reset];
	NSButton *done = [NSButton buttonWithTitle:@"Done" target:self action:@selector(closeControls:)];
	done.bezelStyle = NSBezelStyleRounded;
	done.keyEquivalent = @"\r";
	done.frame = NSMakeRect(516.0, 18.0, 100.0, 30.0);
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
	Platform_InputSetKeyboardSuppressed(1);
	[self.gameWindow addChildWindow:self.panel ordered:NSWindowAbove];
	[self.panel center];
	[self.panel makeKeyAndOrderFront:nil];
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

#include <platform/native_input.h>

#include <macros.h>
#include "psx/libpad.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NATIVE_INPUT_MAX_CONTROLLERS       PLATFORM_INPUT_PAD_COUNT
#define NATIVE_INPUT_PHYSICAL_SLOT_COUNT   2
#define NATIVE_INPUT_PAD_PACKET_BYTES      8
#define NATIVE_INPUT_MULTITAP_HEADER       2
#define NATIVE_INPUT_PAD_DIGITAL           0x41
#define NATIVE_INPUT_PAD_ANALOG            0x73
#define NATIVE_INPUT_PAD_MULTITAP          0x80
#define NATIVE_INPUT_PAD_DISCONNECT        0xff
#define NATIVE_INPUT_AXIS_DEADZONE         500
#define NATIVE_INPUT_MAP_FLAG_AXIS         0x4000
#define NATIVE_INPUT_MAP_FLAG_INVERSE      0x8000
#define NATIVE_INPUT_DEFAULT_KEYBOARD_SLOT 0
#define NATIVE_INPUT_SUBMIT_NAME_KEY_MARKER 0x53
// NOTE(aalhendi): Little-endian tag `CTRI` = CTR native Input snapshot.
#define NATIVE_INPUT_STATE_MAGIC           0x49525443
#define NATIVE_INPUT_STATE_VERSION         1

// NOTE(aalhendi): Native input preserves behavior from PsyCross's
// MIT-licensed pad implementation while moving host ownership into ctr-native.
// See THIRD_PARTY_NOTICES.md.

struct NativeInputKeyboardMapping
{
	s32 id;

	s32 kc_square, kc_circle, kc_triangle, kc_cross;
	s32 kc_square_alt, kc_circle_alt, kc_triangle_alt, kc_cross_alt;

	s32 kc_l1, kc_l2, kc_l3;
	s32 kc_r1, kc_r2, kc_r3;
	s32 kc_l1_alt, kc_r1_alt;

	s32 kc_start, kc_select;
	s32 kc_start_alt, kc_select_alt;

	s32 kc_dpad_left, kc_dpad_right, kc_dpad_up, kc_dpad_down;
	s32 kc_dpad_left_alt, kc_dpad_right_alt, kc_dpad_up_alt, kc_dpad_down_alt;
};

struct NativeInputControllerMapping
{
	s32 id;

	s32 gc_square, gc_circle, gc_triangle, gc_cross;

	s32 gc_l1, gc_l2, gc_l3;
	s32 gc_r1, gc_r2, gc_r3;

	s32 gc_start, gc_select;

	s32 gc_dpad_left, gc_dpad_right, gc_dpad_up, gc_dpad_down;

	s32 gc_axis_left_x, gc_axis_left_y;
	s32 gc_axis_right_x, gc_axis_right_y;
};

struct NativeInputController
{
	SDL_JoystickID instanceId;
	SDL_Gamepad *controller;
	s32 analogEnabled;
	s32 switchingAnalog;
	struct PlatformInputPadSnapshot snapshot;
};

struct NativeInputControllerStateSnapshot
{
	struct PlatformInputPadSnapshot snapshot;
	s32 analogEnabled;
	s32 switchingAnalog;
	s32 controllerToSlotMapping;
};

struct NativeInputStateSnapshot
{
	u32 magic;
	u32 version;
	u32 size;
	s32 keyboardControllerSlot;
	s32 lastActiveControllerSlot;
	s32 installedSnapshotsActive;
	struct PlatformInputPadSnapshot installedSnapshots[NATIVE_INPUT_MAX_CONTROLLERS];
	struct NativeInputControllerStateSnapshot controllers[NATIVE_INPUT_MAX_CONTROLLERS];
};

global_variable struct NativeInputControllerMapping s_controllerMapping;
global_variable struct NativeInputKeyboardMapping s_keyboardMapping;
global_variable s32 s_controllerToSlotMapping[NATIVE_INPUT_MAX_CONTROLLERS] = {-1, -1, -1, -1};

global_variable struct NativeInputController s_controllers[NATIVE_INPUT_MAX_CONTROLLERS];
global_variable struct PlatformInputPadSnapshot s_installedSnapshots[NATIVE_INPUT_MAX_CONTROLLERS];
global_variable u8 *s_padSlotData[NATIVE_INPUT_PHYSICAL_SLOT_COUNT];
global_variable const bool *s_keyboardState;
global_variable s32 s_inputInitialized;
global_variable s32 s_installedSnapshotsActive;
global_variable s32 s_keyboardControllerSlot = NATIVE_INPUT_DEFAULT_KEYBOARD_SLOT;
global_variable s32 s_lastActiveControllerSlot = -1;
global_variable s32 s_submitNameKey;
// SDL can deliver key-down and key-up between two ~29.9 Hz retail pad polls.
// Keep active-low button edges for exactly one snapshot so quick taps survive.
// This is host input transport state and is intentionally not serialized.
global_variable u16 s_keyboardLatchedButtons = 0xffff;

extern s32 g_padCommEnable;

internal u16 NativeInput_GetSnapshotButtons(const struct PlatformInputPadSnapshot *snapshot)
{
	return (u16)(snapshot->buttons[0] | (snapshot->buttons[1] << 8));
}

internal void NativeInput_SetSnapshotButtons(struct PlatformInputPadSnapshot *snapshot, u16 buttons)
{
	snapshot->buttons[0] = (u8)(buttons & 0xff);
	snapshot->buttons[1] = (u8)(buttons >> 8);
}

internal void NativeInput_SetSnapshotSubmitNameKey(struct PlatformInputPadSnapshot *snapshot, s32 key)
{
	if (snapshot == NULL)
	{
		return;
	}

	snapshot->reserved[0] = NATIVE_INPUT_SUBMIT_NAME_KEY_MARKER;
	snapshot->reserved[1] = (u8)(key & 0xff);
	snapshot->reserved[2] = (u8)((u32)key >> 8);
}

internal s32 NativeInput_GetSnapshotSubmitNameKey(const struct PlatformInputPadSnapshot *snapshot)
{
	if ((snapshot == NULL) || (snapshot->reserved[0] != NATIVE_INPUT_SUBMIT_NAME_KEY_MARKER))
	{
		return 0;
	}

	return (s32)(snapshot->reserved[1] | ((u32)snapshot->reserved[2] << 8));
}

internal void NativeInput_ResetSnapshot(s32 slot)
{
	struct PlatformInputPadSnapshot *snapshot = &s_controllers[slot].snapshot;

	snapshot->connected = slot == 0;
	snapshot->status = snapshot->connected ? 0 : NATIVE_INPUT_PAD_DISCONNECT;
	snapshot->id = snapshot->connected ? NATIVE_INPUT_PAD_DIGITAL : NATIVE_INPUT_PAD_DISCONNECT;
	NativeInput_SetSnapshotButtons(snapshot, 0xffff);
	snapshot->analog[0] = 0x80;
	snapshot->analog[1] = 0x80;
	snapshot->analog[2] = 0x80;
	snapshot->analog[3] = 0x80;
	memset(snapshot->reserved, 0, sizeof(snapshot->reserved));
}

internal s32 NativeInput_IsValidControllerSlot(s32 slot)
{
	return (slot >= 0) && (slot < NATIVE_INPUT_MAX_CONTROLLERS);
}

internal s32 NativeInput_NextControllerSlot(s32 slot)
{
	slot++;
	if (slot >= NATIVE_INPUT_MAX_CONTROLLERS)
	{
		slot = 0;
	}

	return slot;
}

internal void NativeInput_MoveKeyboardOffControllerSlot(s32 slot)
{
	if (s_keyboardControllerSlot == slot)
	{
		s_keyboardControllerSlot = NativeInput_NextControllerSlot(s_keyboardControllerSlot);
	}
}

internal void NativeInput_MakeDisconnectedSnapshot(struct PlatformInputPadSnapshot *snapshot)
{
	if (snapshot == NULL)
	{
		return;
	}

	snapshot->connected = 0;
	snapshot->status = NATIVE_INPUT_PAD_DISCONNECT;
	snapshot->id = NATIVE_INPUT_PAD_DISCONNECT;
	NativeInput_SetSnapshotButtons(snapshot, 0xffff);
	snapshot->analog[0] = 0x80;
	snapshot->analog[1] = 0x80;
	snapshot->analog[2] = 0x80;
	snapshot->analog[3] = 0x80;
	memset(snapshot->reserved, 0, sizeof(snapshot->reserved));
}

internal void NativeInput_WritePadPacket(u8 *dst, const struct PlatformInputPadSnapshot *snapshot)
{
	if ((dst == NULL) || (snapshot == NULL))
	{
		return;
	}

	dst[0] = snapshot->status;
	dst[1] = snapshot->id;
	dst[2] = snapshot->buttons[0];
	dst[3] = snapshot->buttons[1];
	dst[4] = snapshot->analog[0];
	dst[5] = snapshot->analog[1];
	dst[6] = snapshot->analog[2];
	dst[7] = snapshot->analog[3];
}

internal s32 NativeInput_UseMultitapBus(void)
{
	s32 slot;

	for (slot = NATIVE_INPUT_PHYSICAL_SLOT_COUNT; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		if (s_controllers[slot].snapshot.connected != 0)
		{
			return 1;
		}
	}

	return 0;
}

internal void NativeInput_WritePadBus(void)
{
	u8 *slot0 = s_padSlotData[0];
	u8 *slot1 = s_padSlotData[1];
	s32 useMultitap = NativeInput_UseMultitapBus();
	s32 slot;

	if (slot0 != NULL)
	{
		if (useMultitap != 0)
		{
			slot0[0] = 0;
			slot0[1] = NATIVE_INPUT_PAD_MULTITAP;
			for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
			{
				NativeInput_WritePadPacket(&slot0[NATIVE_INPUT_MULTITAP_HEADER + (slot * NATIVE_INPUT_PAD_PACKET_BYTES)], &s_controllers[slot].snapshot);
			}
		}
		else
		{
			NativeInput_WritePadPacket(slot0, &s_controllers[0].snapshot);
		}
	}

	if (slot1 != NULL)
	{
		if (useMultitap != 0)
		{
			struct PlatformInputPadSnapshot disconnected;

			NativeInput_MakeDisconnectedSnapshot(&disconnected);
			NativeInput_WritePadPacket(slot1, &disconnected);
		}
		else
		{
			NativeInput_WritePadPacket(slot1, &s_controllers[1].snapshot);
		}
	}
}

internal void NativeInput_WriteInstalledSnapshots(void)
{
	s32 slot;

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		s_controllers[slot].snapshot = s_installedSnapshots[slot];
	}

	NativeInput_WritePadBus();
}

internal void NativeInput_DefaultMappings(void)
{
	s_keyboardMapping.kc_square = SDL_SCANCODE_X;
	s_keyboardMapping.kc_circle = SDL_SCANCODE_V;
	s_keyboardMapping.kc_triangle = SDL_SCANCODE_Z;
	s_keyboardMapping.kc_cross = SDL_SCANCODE_C;
	s_keyboardMapping.kc_square_alt = SDL_SCANCODE_J;
	s_keyboardMapping.kc_circle_alt = SDL_SCANCODE_L;
	s_keyboardMapping.kc_triangle_alt = SDL_SCANCODE_I;
	s_keyboardMapping.kc_cross_alt = SDL_SCANCODE_K;

	s_keyboardMapping.kc_l1 = SDL_SCANCODE_LSHIFT;
	s_keyboardMapping.kc_l2 = SDL_SCANCODE_LCTRL;
	s_keyboardMapping.kc_l3 = SDL_SCANCODE_LEFTBRACKET;

	s_keyboardMapping.kc_r1 = SDL_SCANCODE_RSHIFT;
	s_keyboardMapping.kc_r2 = SDL_SCANCODE_RCTRL;
	s_keyboardMapping.kc_r3 = SDL_SCANCODE_RIGHTBRACKET;
	s_keyboardMapping.kc_l1_alt = SDL_SCANCODE_Q;
	s_keyboardMapping.kc_r1_alt = SDL_SCANCODE_E;

	s_keyboardMapping.kc_dpad_up = SDL_SCANCODE_UP;
	s_keyboardMapping.kc_dpad_down = SDL_SCANCODE_DOWN;
	s_keyboardMapping.kc_dpad_left = SDL_SCANCODE_LEFT;
	s_keyboardMapping.kc_dpad_right = SDL_SCANCODE_RIGHT;
	s_keyboardMapping.kc_dpad_up_alt = SDL_SCANCODE_W;
	s_keyboardMapping.kc_dpad_down_alt = SDL_SCANCODE_S;
	s_keyboardMapping.kc_dpad_left_alt = SDL_SCANCODE_A;
	s_keyboardMapping.kc_dpad_right_alt = SDL_SCANCODE_D;

	s_keyboardMapping.kc_select = SDL_SCANCODE_SPACE;
	s_keyboardMapping.kc_start = SDL_SCANCODE_RETURN;
	s_keyboardMapping.kc_select_alt = SDL_SCANCODE_TAB;
	s_keyboardMapping.kc_start_alt = SDL_SCANCODE_P;

	s_controllerMapping.gc_square = SDL_GAMEPAD_BUTTON_WEST;
	s_controllerMapping.gc_circle = SDL_GAMEPAD_BUTTON_EAST;
	s_controllerMapping.gc_triangle = SDL_GAMEPAD_BUTTON_NORTH;
	s_controllerMapping.gc_cross = SDL_GAMEPAD_BUTTON_SOUTH;

	s_controllerMapping.gc_l1 = SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
	s_controllerMapping.gc_l2 = SDL_GAMEPAD_AXIS_LEFT_TRIGGER | NATIVE_INPUT_MAP_FLAG_AXIS;
	s_controllerMapping.gc_l3 = SDL_GAMEPAD_BUTTON_LEFT_STICK;

	s_controllerMapping.gc_r1 = SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
	s_controllerMapping.gc_r2 = SDL_GAMEPAD_AXIS_RIGHT_TRIGGER | NATIVE_INPUT_MAP_FLAG_AXIS;
	s_controllerMapping.gc_r3 = SDL_GAMEPAD_BUTTON_RIGHT_STICK;

	s_controllerMapping.gc_dpad_up = SDL_GAMEPAD_BUTTON_DPAD_UP;
	s_controllerMapping.gc_dpad_down = SDL_GAMEPAD_BUTTON_DPAD_DOWN;
	s_controllerMapping.gc_dpad_left = SDL_GAMEPAD_BUTTON_DPAD_LEFT;
	s_controllerMapping.gc_dpad_right = SDL_GAMEPAD_BUTTON_DPAD_RIGHT;

	s_controllerMapping.gc_select = SDL_GAMEPAD_BUTTON_BACK;
	s_controllerMapping.gc_start = SDL_GAMEPAD_BUTTON_START;

	s_controllerMapping.gc_axis_left_x = SDL_GAMEPAD_AXIS_LEFTX | NATIVE_INPUT_MAP_FLAG_AXIS;
	s_controllerMapping.gc_axis_left_y = SDL_GAMEPAD_AXIS_LEFTY | NATIVE_INPUT_MAP_FLAG_AXIS;
	s_controllerMapping.gc_axis_right_x = SDL_GAMEPAD_AXIS_RIGHTX | NATIVE_INPUT_MAP_FLAG_AXIS;
	s_controllerMapping.gc_axis_right_y = SDL_GAMEPAD_AXIS_RIGHTY | NATIVE_INPUT_MAP_FLAG_AXIS;
}

internal u16 NativeInput_KeyboardButtonBit(s32 key)
{
	const struct NativeInputKeyboardMapping *mapping = &s_keyboardMapping;

	if ((key == mapping->kc_square) || (key == mapping->kc_square_alt))
		return 0x8000;
	if ((key == mapping->kc_circle) || (key == mapping->kc_circle_alt))
		return 0x2000;
	if ((key == mapping->kc_triangle) || (key == mapping->kc_triangle_alt))
		return 0x1000;
	if ((key == mapping->kc_cross) || (key == mapping->kc_cross_alt))
		return 0x4000;
	if ((key == mapping->kc_l1) || (key == mapping->kc_l1_alt))
		return 0x400;
	if (key == mapping->kc_l2)
		return 0x100;
	if (key == mapping->kc_l3)
		return 0x2;
	if ((key == mapping->kc_r1) || (key == mapping->kc_r1_alt))
		return 0x800;
	if (key == mapping->kc_r2)
		return 0x200;
	if (key == mapping->kc_r3)
		return 0x4;
	if ((key == mapping->kc_dpad_up) || (key == mapping->kc_dpad_up_alt))
		return 0x10;
	if ((key == mapping->kc_dpad_down) || (key == mapping->kc_dpad_down_alt))
		return 0x40;
	if ((key == mapping->kc_dpad_left) || (key == mapping->kc_dpad_left_alt))
		return 0x80;
	if ((key == mapping->kc_dpad_right) || (key == mapping->kc_dpad_right_alt))
		return 0x20;
	if ((key == mapping->kc_select) || (key == mapping->kc_select_alt))
		return 0x1;
	if ((key == mapping->kc_start) || (key == mapping->kc_start_alt))
		return 0x8;

	return 0;
}

internal void NativeInput_ClearKeyboardLatch(void)
{
	s_keyboardLatchedButtons = 0xffff;
}

void Platform_InputKeyboardEvent(int key, int down)
{
	u16 buttonBit;

	if (down == 0)
	{
		return;
	}

	buttonBit = NativeInput_KeyboardButtonBit(key);
	if (buttonBit != 0)
	{
		s_keyboardLatchedButtons &= (u16)~buttonBit;
	}
}

internal s32 NativeInput_ControllerButtonState(SDL_Gamepad *controller, s32 buttonOrAxis)
{
	if (controller == NULL)
	{
		return 0;
	}

	if ((buttonOrAxis & NATIVE_INPUT_MAP_FLAG_AXIS) != 0)
	{
		s32 axis = buttonOrAxis & ~(NATIVE_INPUT_MAP_FLAG_AXIS | NATIVE_INPUT_MAP_FLAG_INVERSE);
		s32 value = SDL_GetGamepadAxis(controller, (SDL_GamepadAxis)axis);

		if ((abs(value) > NATIVE_INPUT_AXIS_DEADZONE) && ((buttonOrAxis & NATIVE_INPUT_MAP_FLAG_INVERSE) != 0))
		{
			value *= -1;
		}

		return value;
	}

	if (buttonOrAxis < 0)
	{
		return 0;
	}

	return SDL_GetGamepadButton(controller, (SDL_GamepadButton)buttonOrAxis) * 32767;
}

internal u8 NativeInput_AxisToByte(s32 axis)
{
	s32 value = (axis / 256) + 128;

	if (value < 0)
	{
		return 0;
	}

	if (value > 0xff)
	{
		return 0xff;
	}

	return (u8)value;
}

internal s32 NativeInput_AxisIsActive(s32 axis)
{
	return abs(axis) > NATIVE_INPUT_AXIS_DEADZONE;
}

internal void NativeInput_ApplyController(s32 slot)
{
	struct NativeInputController *nativeController = &s_controllers[slot];
	struct PlatformInputPadSnapshot *snapshot = &nativeController->snapshot;
	const struct NativeInputControllerMapping *mapping = &s_controllerMapping;
	SDL_Gamepad *controller = nativeController->controller;
	u16 buttons = 0xffff;
	s32 rightX;
	s32 rightY;
	s32 leftX;
	s32 leftY;

	if ((controller == NULL) || (SDL_GamepadConnected(controller) == 0))
	{
		return;
	}

	snapshot->connected = 1;
	snapshot->status = 0;
	snapshot->id = nativeController->analogEnabled ? NATIVE_INPUT_PAD_ANALOG : NATIVE_INPUT_PAD_DIGITAL;

	if (NativeInput_ControllerButtonState(controller, mapping->gc_square) > 16384)
	{
		buttons &= ~0x8000;
	}
	if (NativeInput_ControllerButtonState(controller, mapping->gc_circle) > 16384)
	{
		buttons &= ~0x2000;
	}
	if (NativeInput_ControllerButtonState(controller, mapping->gc_triangle) > 16384)
	{
		buttons &= ~0x1000;
	}
	if (NativeInput_ControllerButtonState(controller, mapping->gc_cross) > 16384)
	{
		buttons &= ~0x4000;
	}
	if (NativeInput_ControllerButtonState(controller, mapping->gc_l1) > 16384)
	{
		buttons &= ~0x400;
	}
	if (NativeInput_ControllerButtonState(controller, mapping->gc_r1) > 16384)
	{
		buttons &= ~0x800;
	}
	if (NativeInput_ControllerButtonState(controller, mapping->gc_l2) > 16384)
	{
		buttons &= ~0x100;
	}
	if (NativeInput_ControllerButtonState(controller, mapping->gc_r2) > 16384)
	{
		buttons &= ~0x200;
	}
	if (NativeInput_ControllerButtonState(controller, mapping->gc_dpad_up) > 16384)
	{
		buttons &= ~0x10;
	}
	if (NativeInput_ControllerButtonState(controller, mapping->gc_dpad_down) > 16384)
	{
		buttons &= ~0x40;
	}
	if (NativeInput_ControllerButtonState(controller, mapping->gc_dpad_left) > 16384)
	{
		buttons &= ~0x80;
	}
	if (NativeInput_ControllerButtonState(controller, mapping->gc_dpad_right) > 16384)
	{
		buttons &= ~0x20;
	}
	if (NativeInput_ControllerButtonState(controller, mapping->gc_l3) > 16384)
	{
		buttons &= ~0x2;
	}
	if (NativeInput_ControllerButtonState(controller, mapping->gc_r3) > 16384)
	{
		buttons &= ~0x4;
	}
	if (NativeInput_ControllerButtonState(controller, mapping->gc_select) > 16384)
	{
		buttons &= ~0x1;
	}
	if (NativeInput_ControllerButtonState(controller, mapping->gc_start) > 16384)
	{
		buttons &= ~0x8;
	}

	rightX = NativeInput_ControllerButtonState(controller, mapping->gc_axis_right_x);
	rightY = NativeInput_ControllerButtonState(controller, mapping->gc_axis_right_y);
	leftX = NativeInput_ControllerButtonState(controller, mapping->gc_axis_left_x);
	leftY = NativeInput_ControllerButtonState(controller, mapping->gc_axis_left_y);

	if ((buttons != 0xffff) || NativeInput_AxisIsActive(rightX) || NativeInput_AxisIsActive(rightY) || NativeInput_AxisIsActive(leftX) ||
	    NativeInput_AxisIsActive(leftY))
	{
		s_lastActiveControllerSlot = slot;
	}

	if (((buttons & 0x1) == 0) && ((buttons & 0x8) == 0))
	{
		buttons = 0xffff;
		if (nativeController->switchingAnalog == 0)
		{
			nativeController->analogEnabled = nativeController->analogEnabled == 0;
		}
		nativeController->switchingAnalog = 1;
	}
	else
	{
		nativeController->switchingAnalog = 0;
	}

	NativeInput_SetSnapshotButtons(snapshot, buttons);
	snapshot->analog[0] = NativeInput_AxisToByte(rightX);
	snapshot->analog[1] = NativeInput_AxisToByte(rightY);
	snapshot->analog[2] = NativeInput_AxisToByte(leftX);
	snapshot->analog[3] = NativeInput_AxisToByte(leftY);
}

internal u16 NativeInput_ReadKeyboard(void)
{
	const struct NativeInputKeyboardMapping *mapping = &s_keyboardMapping;
	u16 buttons = s_keyboardLatchedButtons;

	if (s_keyboardState == NULL)
	{
		return buttons;
	}

	if (s_keyboardState[mapping->kc_square] || s_keyboardState[mapping->kc_square_alt])
	{
		buttons &= ~0x8000;
	}
	if (s_keyboardState[mapping->kc_circle] || s_keyboardState[mapping->kc_circle_alt])
	{
		buttons &= ~0x2000;
	}
	if (s_keyboardState[mapping->kc_triangle] || s_keyboardState[mapping->kc_triangle_alt])
	{
		buttons &= ~0x1000;
	}
	if (s_keyboardState[mapping->kc_cross] || s_keyboardState[mapping->kc_cross_alt])
	{
		buttons &= ~0x4000;
	}
	if (s_keyboardState[mapping->kc_l1] || s_keyboardState[mapping->kc_l1_alt])
	{
		buttons &= ~0x400;
	}
	if (s_keyboardState[mapping->kc_l2])
	{
		buttons &= ~0x100;
	}
	if (s_keyboardState[mapping->kc_l3])
	{
		buttons &= ~0x2;
	}
	if (s_keyboardState[mapping->kc_r1] || s_keyboardState[mapping->kc_r1_alt])
	{
		buttons &= ~0x800;
	}
	if (s_keyboardState[mapping->kc_r2])
	{
		buttons &= ~0x200;
	}
	if (s_keyboardState[mapping->kc_r3])
	{
		buttons &= ~0x4;
	}
	if (s_keyboardState[mapping->kc_dpad_up] || s_keyboardState[mapping->kc_dpad_up_alt])
	{
		buttons &= ~0x10;
	}
	if (s_keyboardState[mapping->kc_dpad_down] || s_keyboardState[mapping->kc_dpad_down_alt])
	{
		buttons &= ~0x40;
	}
	if (s_keyboardState[mapping->kc_dpad_left] || s_keyboardState[mapping->kc_dpad_left_alt])
	{
		buttons &= ~0x80;
	}
	if (s_keyboardState[mapping->kc_dpad_right] || s_keyboardState[mapping->kc_dpad_right_alt])
	{
		buttons &= ~0x20;
	}
	if (s_keyboardState[mapping->kc_select] || s_keyboardState[mapping->kc_select_alt])
	{
		buttons &= ~0x1;
	}
	if (s_keyboardState[mapping->kc_start] || s_keyboardState[mapping->kc_start_alt])
	{
		buttons &= ~0x8;
	}

	return buttons;
}

internal u16 NativeInput_ConsumeKeyboard(void)
{
	u16 buttons = NativeInput_ReadKeyboard();

	NativeInput_ClearKeyboardLatch();
	return buttons;
}

internal s32 NativeInput_KeyboardSuppressed(void)
{
	if (s_keyboardState == NULL)
	{
		return 0;
	}

	return s_keyboardState[SDL_SCANCODE_RALT] || s_keyboardState[SDL_SCANCODE_LALT];
}

internal void NativeInput_ApplyKeyboard(s32 slot, u16 keyboardButtons)
{
	struct PlatformInputPadSnapshot *snapshot = &s_controllers[slot].snapshot;
	u16 buttons;

	if (slot != s_keyboardControllerSlot)
	{
		return;
	}

	if (snapshot->connected == 0)
	{
		snapshot->connected = 1;
		snapshot->status = 0;
		snapshot->id = NATIVE_INPUT_PAD_DIGITAL;
	}

	buttons = NativeInput_GetSnapshotButtons(snapshot);
	NativeInput_SetSnapshotButtons(snapshot, buttons & keyboardButtons);
}

internal s32 NativeInput_FindActiveControllerSlot(void)
{
	s32 slot;

	if (NativeInput_IsValidControllerSlot(s_lastActiveControllerSlot) && (s_controllers[s_lastActiveControllerSlot].controller != NULL))
	{
		return s_lastActiveControllerSlot;
	}

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		if (s_controllers[slot].controller != NULL)
		{
			return slot;
		}
	}

	return -1;
}

internal void NativeInput_SwapControllerSlots(s32 slotA, s32 slotB)
{
	struct NativeInputController controller;
	s32 mapping;

	if (!NativeInput_IsValidControllerSlot(slotA) || !NativeInput_IsValidControllerSlot(slotB) || (slotA == slotB))
	{
		return;
	}

	controller = s_controllers[slotA];
	s_controllers[slotA] = s_controllers[slotB];
	s_controllers[slotB] = controller;

	mapping = s_controllerToSlotMapping[slotA];
	s_controllerToSlotMapping[slotA] = s_controllerToSlotMapping[slotB];
	s_controllerToSlotMapping[slotB] = mapping;

	if (s_lastActiveControllerSlot == slotA)
	{
		s_lastActiveControllerSlot = slotB;
	}
	else if (s_lastActiveControllerSlot == slotB)
	{
		s_lastActiveControllerSlot = slotA;
	}
}

internal s32 NativeInput_FindSlotForDeviceIndex(Sint32 deviceIndex)
{
	s32 slot;

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		if (s_controllerToSlotMapping[slot] == deviceIndex)
		{
			return slot;
		}
	}

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		if ((s_controllerToSlotMapping[slot] < 0) && (s_controllers[slot].controller == NULL))
		{
			return slot;
		}
	}

	return -1;
}

internal void NativeInput_CloseController(s32 slot)
{
	struct NativeInputController *controller;

	if ((slot < 0) || (slot >= NATIVE_INPUT_MAX_CONTROLLERS))
	{
		return;
	}

	controller = &s_controllers[slot];
	if (controller->controller != NULL)
	{
		SDL_CloseGamepad(controller->controller);
	}

	controller->controller = NULL;
	controller->instanceId = -1;
	controller->analogEnabled = 0;
	controller->switchingAnalog = 0;
	s_controllerToSlotMapping[slot] = -1;

	if (s_lastActiveControllerSlot == slot)
	{
		s_lastActiveControllerSlot = -1;
	}
}

internal void NativeInput_OpenController(SDL_JoystickID instanceId, s32 slot)
{
	struct NativeInputController *controller;
	SDL_Joystick *joystick;

	if ((slot < 0) || (slot >= NATIVE_INPUT_MAX_CONTROLLERS))
	{
		return;
	}

	if (SDL_IsGamepad(instanceId) == 0)
	{
		return;
	}

	controller = &s_controllers[slot];
	if (controller->controller != NULL)
	{
		return;
	}

	controller->controller = SDL_OpenGamepad(instanceId);
	if (controller->controller == NULL)
	{
		return;
	}

	joystick = SDL_GetGamepadJoystick(controller->controller);
	controller->instanceId = joystick != NULL ? SDL_GetJoystickID(joystick) : instanceId;
	controller->analogEnabled = 1;
	controller->switchingAnalog = 0;
	s_controllerToSlotMapping[slot] = controller->instanceId;
	NativeInput_MoveKeyboardOffControllerSlot(slot);
}

internal void NativeInput_OpenKnownControllers(void)
{
	SDL_JoystickID *gamepads;
	s32 count = 0;
	s32 i;

	gamepads = SDL_GetGamepads(&count);
	for (i = 0; i < count; i++)
	{
		s32 slot = NativeInput_FindSlotForDeviceIndex(gamepads[i]);

		if (slot >= 0)
		{
			NativeInput_OpenController(gamepads[i], slot);
		}
	}
	SDL_free(gamepads);
}

int Platform_InputInit(void)
{
	s32 slot;

	if (s_inputInitialized != 0)
	{
		return 1;
	}

	memset(s_controllers, 0, sizeof(s_controllers));
	memset(s_padSlotData, 0, sizeof(s_padSlotData));
	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		s_controllers[slot].instanceId = -1;
		NativeInput_ResetSnapshot(slot);
		s_installedSnapshots[slot] = s_controllers[slot].snapshot;
	}

	NativeInput_DefaultMappings();
	s_keyboardControllerSlot = NATIVE_INPUT_DEFAULT_KEYBOARD_SLOT;
	s_lastActiveControllerSlot = -1;
	s_installedSnapshotsActive = 0;
	s_keyboardState = SDL_GetKeyboardState(NULL);
	s_submitNameKey = 0;
	NativeInput_ClearKeyboardLatch();

	if (SDL_InitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC) == 0)
	{
		fprintf(stderr, "[CTR Native] Failed to initialise SDL input subsystem: %s\n", SDL_GetError());
		return 0;
	}

	SDL_AddGamepadMappingsFromFile("gamecontrollerdb.txt");
	NativeInput_OpenKnownControllers();

	s_inputInitialized = 1;
	return 1;
}

void Platform_InputShutdown(void)
{
	s32 slot;

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		NativeInput_CloseController(slot);
	}

	if (s_inputInitialized != 0)
	{
		SDL_QuitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC);
	}

	s_inputInitialized = 0;
	s_installedSnapshotsActive = 0;
	s_keyboardControllerSlot = NATIVE_INPUT_DEFAULT_KEYBOARD_SLOT;
	s_lastActiveControllerSlot = -1;
	s_submitNameKey = 0;
	NativeInput_ClearKeyboardLatch();
	memset(s_padSlotData, 0, sizeof(s_padSlotData));
	s_keyboardState = NULL;
}

void Platform_InputUpdate(void)
{
	u16 keyboardButtons;
	s32 slot;

	if (s_inputInitialized == 0)
	{
		return;
	}

	if (s_installedSnapshotsActive != 0)
	{
		// NOTE(aalhendi): replay/state installs PSX-shaped pad bytes here;
		// SDL host state is not serialized.
		NativeInput_ClearKeyboardLatch();
		NativeInput_WriteInstalledSnapshots();
		return;
	}

	if (g_padCommEnable == 0)
	{
		NativeInput_ClearKeyboardLatch();
		return;
	}

	SDL_PumpEvents();
	keyboardButtons = NativeInput_ConsumeKeyboard();
	if (NativeInput_KeyboardSuppressed())
	{
		keyboardButtons = 0xffff;
	}

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		NativeInput_ResetSnapshot(slot);
		NativeInput_ApplyController(slot);
		NativeInput_ApplyKeyboard(slot, keyboardButtons);
		if (slot == s_keyboardControllerSlot)
		{
			NativeInput_SetSnapshotSubmitNameKey(&s_controllers[slot].snapshot, s_submitNameKey);
		}
	}
	NativeInput_WritePadBus();
}

void Platform_InputSuspend(void)
{
	s32 slot;

	NativeInput_ClearKeyboardLatch();
	s_submitNameKey = 0;

	if (s_inputInitialized == 0)
	{
		return;
	}

	// Publish a released snapshot before UIKit removes CPU time. This prevents
	// a key, button, or axis held at the resign-active boundary from sticking
	// in the PSX-shaped pad bus across foreground restoration.
	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		NativeInput_ResetSnapshot(slot);
	}
	NativeInput_WritePadBus();
}

void Platform_InputResume(void)
{
	// The next ordinary update samples current keyboard/gamepad state. Clear
	// only transport edges accumulated before the foreground boundary.
	NativeInput_ClearKeyboardLatch();
	s_submitNameKey = 0;
}

void Platform_InputControllerAdded(int deviceIndex)
{
	s32 slot;

	if (s_inputInitialized == 0)
	{
		return;
	}

	slot = NativeInput_FindSlotForDeviceIndex(deviceIndex);
	if (slot >= 0)
	{
		NativeInput_OpenController(deviceIndex, slot);
	}
}

void Platform_InputControllerRemoved(int instanceId)
{
	s32 slot;

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		if (s_controllers[slot].instanceId == (SDL_JoystickID)instanceId)
		{
			NativeInput_CloseController(slot);
			return;
		}
	}
}

int Platform_InputCycleKeyboardController(void)
{
	s_keyboardControllerSlot = NativeInput_NextControllerSlot(s_keyboardControllerSlot);
	return s_keyboardControllerSlot + 1;
}

int Platform_InputCycleGamepadController(void)
{
	s32 slot = NativeInput_FindActiveControllerSlot();
	s32 nextSlot;

	if (slot < 0)
	{
		return 0;
	}

	nextSlot = NativeInput_NextControllerSlot(slot);
	NativeInput_SwapControllerSlots(slot, nextSlot);
	NativeInput_WritePadBus();
	return nextSlot + 1;
}

void Platform_InputPadInit(int slot, unsigned char *padData)
{
	if ((slot < 0) || (slot >= NATIVE_INPUT_PHYSICAL_SLOT_COUNT))
	{
		return;
	}

	s_padSlotData[slot] = padData;
	NativeInput_WritePadBus();
}

int Platform_InputPadGetState(int port)
{
	s32 physicalSlot = (port >> 4) & 1;
	s32 tap = port & 3;
	s32 slot;

	if (NativeInput_UseMultitapBus() != 0)
	{
		if (physicalSlot != 0)
		{
			return PadStateDiscon;
		}
		slot = tap;
	}
	else
	{
		if (tap != 0)
		{
			return PadStateDiscon;
		}
		slot = physicalSlot;
	}

	if ((slot < 0) || (slot >= NATIVE_INPUT_MAX_CONTROLLERS))
	{
		return PadStateDiscon;
	}

	return s_controllers[slot].snapshot.connected ? PadStateStable : PadStateDiscon;
}

int Platform_InputCapturePadSnapshots(struct PlatformInputPadSnapshot *dst, int count)
{
	s32 slot;

	if ((dst == NULL) || (count < NATIVE_INPUT_MAX_CONTROLLERS))
	{
		return 0;
	}

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		dst[slot] = s_controllers[slot].snapshot;
	}

	return NATIVE_INPUT_MAX_CONTROLLERS;
}

int Platform_InputInstallPadSnapshots(const struct PlatformInputPadSnapshot *src, int count)
{
	s32 slot;

	if ((src == NULL) || (count < NATIVE_INPUT_MAX_CONTROLLERS))
	{
		return 0;
	}

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		s_installedSnapshots[slot] = src[slot];
	}

	s_installedSnapshotsActive = 1;
	NativeInput_WriteInstalledSnapshots();
	return NATIVE_INPUT_MAX_CONTROLLERS;
}

void Platform_InputClearInstalledPadSnapshots(void)
{
	s_installedSnapshotsActive = 0;
}

int Platform_InputUpgradeLegacySubmitNameSnapshots(struct PlatformInputPadSnapshot *snapshots, int count)
{
	if ((snapshots == NULL) || (count != NATIVE_INPUT_MAX_CONTROLLERS))
	{
		return 0;
	}

	for (s32 slot = 0; slot < count; slot++)
	{
		struct PlatformInputPadSnapshot *snapshot = &snapshots[slot];
		s32 submitNameKey = 0;

		if (snapshot->reserved[0] == NATIVE_INPUT_SUBMIT_NAME_KEY_MARKER)
		{
			continue;
		}

		// Legacy version-2 reports did not serialize host-only name-entry
		// shortcuts. Their keyboard Enter mapping is recoverable from the raw
		// Start bit; other keyboard scancodes are not.
		if ((snapshot->connected != 0) && ((NativeInput_GetSnapshotButtons(snapshot) & 0x8) == 0))
		{
			submitNameKey = SDL_SCANCODE_RETURN;
		}
		NativeInput_SetSnapshotSubmitNameKey(snapshot, submitNameKey);
	}

	return 1;
}

void Platform_InputSetSubmitNameKey(int key, int down)
{
	s_submitNameKey = (down != 0) ? key : 0;
}

int Platform_InputGetSubmitNameKey(void)
{
	s32 slot;
	s32 hasSubmitNameKeyMetadata = 0;

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		const struct PlatformInputPadSnapshot *snapshot = &s_controllers[slot].snapshot;
		s32 key;

		if (snapshot->reserved[0] != NATIVE_INPUT_SUBMIT_NAME_KEY_MARKER)
		{
			continue;
		}
		hasSubmitNameKeyMetadata = 1;
		key = NativeInput_GetSnapshotSubmitNameKey(snapshot);

		if (key != 0)
		{
			return key;
		}
	}

	// Version-2 replay records predate native shortcut metadata. Enter was
	// already mapped to raw Start, so recover that one legacy shortcut only
	// while replay/state snapshots are installed. Live gamepad Start remains
	// retail input and is not treated as a host keyboard shortcut.
	if ((s_installedSnapshotsActive != 0) && (hasSubmitNameKeyMetadata == 0))
	{
		for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
		{
			const struct PlatformInputPadSnapshot *snapshot = &s_controllers[slot].snapshot;

			if ((snapshot->connected != 0) && ((NativeInput_GetSnapshotButtons(snapshot) & 0x8) == 0))
			{
				return SDL_SCANCODE_RETURN;
			}
		}
	}

	return 0;
}

struct NativeInputVirtualRumbleProbe
{
	u16 lowFrequency;
	u16 highFrequency;
	s32 callCount;
};

internal bool SDLCALL NativeInput_VirtualRumble(void *userdata, Uint16 lowFrequency, Uint16 highFrequency)
{
	struct NativeInputVirtualRumbleProbe *probe = userdata;

	if (probe != NULL)
	{
		probe->lowFrequency = lowFrequency;
		probe->highFrequency = highFrequency;
		probe->callCount++;
	}

	return true;
}

internal s32 NativeInput_RunVirtualControllerSelfTest(void)
{
	struct NativeInputVirtualRumbleProbe rumbleProbe = {0};
	SDL_VirtualJoystickDesc virtualDesc;
	SDL_JoystickID virtualId = 0;
	SDL_Joystick *virtualJoystick = NULL;
	const char *failure = NULL;
	unsigned char rumbleTable[2] = {0x40, 0x80};
	u16 buttons;
	s32 openControllerCount;
	s32 slot;

	if (SDL_InitSubSystem(SDL_INIT_GAMEPAD) == 0)
	{
		fprintf(stderr, "[CTR Input] self-test failed: SDL virtual gamepad init: %s\n", SDL_GetError());
		return 0;
	}

	SDL_INIT_INTERFACE(&virtualDesc);
	virtualDesc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
	virtualDesc.naxes = SDL_GAMEPAD_AXIS_COUNT;
	virtualDesc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
	virtualDesc.name = "CTR Native self-test gamepad";
	virtualDesc.userdata = &rumbleProbe;
	virtualDesc.Rumble = NativeInput_VirtualRumble;

	virtualId = SDL_AttachVirtualJoystick(&virtualDesc);
	if (virtualId == 0)
	{
		fprintf(stderr, "[CTR Input] self-test failed: attach virtual gamepad: %s\n", SDL_GetError());
		failure = "attach virtual gamepad";
		goto CLEANUP;
	}

	s_inputInitialized = 1;
	Platform_InputControllerAdded(virtualId);
	if ((s_controllers[0].controller == NULL) || (s_controllers[0].instanceId != virtualId) ||
	    ((SDL_JoystickID)s_controllerToSlotMapping[0] != virtualId) || (s_keyboardControllerSlot != 1))
	{
		failure = "virtual gamepad slot ownership";
		goto CLEANUP;
	}

	Platform_InputControllerAdded(virtualId);
	openControllerCount = 0;
	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		if (s_controllers[slot].controller != NULL)
		{
			openControllerCount++;
		}
	}
	if (openControllerCount != 1)
	{
		failure = "duplicate controller add opened a second slot";
		goto CLEANUP;
	}

	virtualJoystick = SDL_GetGamepadJoystick(s_controllers[0].controller);
	if ((virtualJoystick == NULL) ||
	    !SDL_SetJoystickVirtualButton(virtualJoystick, SDL_GAMEPAD_BUTTON_SOUTH, true) ||
	    !SDL_SetJoystickVirtualButton(virtualJoystick, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, true) ||
	    !SDL_SetJoystickVirtualAxis(virtualJoystick, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 24576) ||
	    !SDL_SetJoystickVirtualAxis(virtualJoystick, SDL_GAMEPAD_AXIS_LEFTX, -32768) ||
	    !SDL_SetJoystickVirtualAxis(virtualJoystick, SDL_GAMEPAD_AXIS_RIGHTY, 32767))
	{
		failure = "set virtual gamepad state";
		goto CLEANUP;
	}

	SDL_UpdateJoysticks();
	NativeInput_ResetSnapshot(0);
	NativeInput_ApplyController(0);
	buttons = NativeInput_GetSnapshotButtons(&s_controllers[0].snapshot);
	if ((s_controllers[0].snapshot.connected == 0) || (s_controllers[0].snapshot.id != NATIVE_INPUT_PAD_ANALOG) ||
	    (buttons != 0xb5ff) || (s_controllers[0].snapshot.analog[0] != 0x80) ||
	    (s_controllers[0].snapshot.analog[1] != 0xff) || (s_controllers[0].snapshot.analog[2] != 0x00) ||
	    (s_controllers[0].snapshot.analog[3] != 0x80) || (s_lastActiveControllerSlot != 0))
	{
		failure = "virtual gamepad PS1 snapshot";
		goto CLEANUP;
	}

	Platform_InputPadVibrate(0, rumbleTable, sizeof(rumbleTable));
	if ((rumbleProbe.callCount != 1) || (rumbleProbe.lowFrequency != 32640) || (rumbleProbe.highFrequency != 16320))
	{
		failure = "virtual gamepad rumble";
		goto CLEANUP;
	}

	Platform_InputControllerRemoved(virtualId);
	if ((s_controllers[0].controller != NULL) || (s_controllerToSlotMapping[0] != -1))
	{
		failure = "virtual gamepad removal";
		goto CLEANUP;
	}

	Platform_InputControllerAdded(virtualId);
	if ((s_controllers[0].controller == NULL) || ((SDL_JoystickID)s_controllerToSlotMapping[0] != virtualId))
	{
		failure = "virtual gamepad reconnect";
		goto CLEANUP;
	}

CLEANUP:
	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		NativeInput_CloseController(slot);
		NativeInput_ResetSnapshot(slot);
	}
	s_inputInitialized = 0;
	s_keyboardControllerSlot = NATIVE_INPUT_DEFAULT_KEYBOARD_SLOT;
	s_lastActiveControllerSlot = -1;
	if ((virtualId != 0) && !SDL_DetachVirtualJoystick(virtualId) && (failure == NULL))
	{
		failure = "detach virtual gamepad";
	}
	SDL_QuitSubSystem(SDL_INIT_GAMEPAD);

	if (failure != NULL)
	{
		fprintf(stderr, "[CTR Input] self-test failed: %s\n", failure);
		return 0;
	}

	return 1;
}

int Platform_InputRunSelfTest(void)
{
	const struct
	{
		s32 key;
		u16 buttonBit;
	} aliasExpectations[] = {
		{SDL_SCANCODE_J, 0x8000},
		{SDL_SCANCODE_L, 0x2000},
		{SDL_SCANCODE_I, 0x1000},
		{SDL_SCANCODE_K, 0x4000},
		{SDL_SCANCODE_Q, 0x400},
		{SDL_SCANCODE_E, 0x800},
		{SDL_SCANCODE_W, 0x10},
		{SDL_SCANCODE_S, 0x40},
		{SDL_SCANCODE_A, 0x80},
		{SDL_SCANCODE_D, 0x20},
		{SDL_SCANCODE_TAB, 0x1},
		{SDL_SCANCODE_P, 0x8},
	};
	struct PlatformInputPadSnapshot migrationSnapshots[NATIVE_INPUT_MAX_CONTROLLERS];
	struct PlatformInputPadSnapshot *snapshot;
	bool keyboardState[SDL_SCANCODE_COUNT];
	u16 heldButtons;
	u16 latchedButtons;
	s32 slot;

	memset(s_controllers, 0, sizeof(s_controllers));
	NativeInput_DefaultMappings();
	s_keyboardState = NULL;
	NativeInput_ClearKeyboardLatch();
	s_keyboardControllerSlot = NATIVE_INPUT_DEFAULT_KEYBOARD_SLOT;
	s_installedSnapshotsActive = 0;
	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		s_controllerToSlotMapping[slot] = -1;
		NativeInput_ResetSnapshot(slot);
	}

	snapshot = &s_controllers[NATIVE_INPUT_DEFAULT_KEYBOARD_SLOT].snapshot;
	NativeInput_SetSnapshotSubmitNameKey(snapshot, SDL_SCANCODE_A);
	if (Platform_InputGetSubmitNameKey() != SDL_SCANCODE_A)
	{
		fprintf(stderr, "[CTR Input] self-test failed: snapshot metadata key\n");
		return 1;
	}

	NativeInput_SetSnapshotSubmitNameKey(snapshot, 0);
	NativeInput_SetSnapshotButtons(snapshot, 0xfff7);
	s_installedSnapshotsActive = 1;
	if (Platform_InputGetSubmitNameKey() != 0)
	{
		fprintf(stderr, "[CTR Input] self-test failed: metadata did not preserve retail Start\n");
		return 1;
	}

	memset(snapshot->reserved, 0, sizeof(snapshot->reserved));
	if (Platform_InputGetSubmitNameKey() != SDL_SCANCODE_RETURN)
	{
		fprintf(stderr, "[CTR Input] self-test failed: legacy replay Enter fallback\n");
		return 1;
	}
	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		migrationSnapshots[slot] = s_controllers[slot].snapshot;
	}
	if (!Platform_InputUpgradeLegacySubmitNameSnapshots(migrationSnapshots, NATIVE_INPUT_MAX_CONTROLLERS) ||
	    (NativeInput_GetSnapshotSubmitNameKey(&migrationSnapshots[0]) != SDL_SCANCODE_RETURN))
	{
		fprintf(stderr, "[CTR Input] self-test failed: legacy replay metadata migration\n");
		return 1;
	}

	NativeInput_SetSnapshotSubmitNameKey(snapshot, 0);
	s_installedSnapshotsActive = 0;
	if (Platform_InputGetSubmitNameKey() != 0)
	{
		fprintf(stderr, "[CTR Input] self-test failed: live gamepad Start became keyboard Enter\n");
		return 1;
	}

	Platform_InputKeyboardEvent(SDL_SCANCODE_C, 1);
	Platform_InputKeyboardEvent(SDL_SCANCODE_RIGHT, 1);
	Platform_InputKeyboardEvent(SDL_SCANCODE_C, 0);
	Platform_InputKeyboardEvent(SDL_SCANCODE_RIGHT, 0);
	latchedButtons = NativeInput_ConsumeKeyboard();
	if (((latchedButtons & 0x4000) != 0) || ((latchedButtons & 0x20) != 0))
	{
		fprintf(stderr, "[CTR Input] self-test failed: key-down tap was not latched\n");
		return 1;
	}
	if (NativeInput_ConsumeKeyboard() != 0xffff)
	{
		fprintf(stderr, "[CTR Input] self-test failed: quick tap survived more than one snapshot\n");
		return 1;
	}

	for (slot = 0; slot < (s32)(sizeof(aliasExpectations) / sizeof(aliasExpectations[0])); slot++)
	{
		if (NativeInput_KeyboardButtonBit(aliasExpectations[slot].key) != aliasExpectations[slot].buttonBit)
		{
			fprintf(stderr, "[CTR Input] self-test failed: keyboard alias index %d\n", slot);
			return 1;
		}
	}

	memset(keyboardState, 0, sizeof(keyboardState));
	keyboardState[SDL_SCANCODE_K] = true;
	keyboardState[SDL_SCANCODE_D] = true;
	keyboardState[SDL_SCANCODE_E] = true;
	s_keyboardState = keyboardState;
	heldButtons = NativeInput_ReadKeyboard();
	s_keyboardState = NULL;
	if (((heldButtons & 0x4000) != 0) || ((heldButtons & 0x20) != 0) || ((heldButtons & 0x800) != 0))
	{
		fprintf(stderr, "[CTR Input] self-test failed: held keyboard aliases k+d+e\n");
		return 1;
	}

	Platform_InputKeyboardEvent(SDL_SCANCODE_K, 1);
	Platform_InputKeyboardEvent(SDL_SCANCODE_D, 1);
	Platform_InputKeyboardEvent(SDL_SCANCODE_K, 0);
	Platform_InputKeyboardEvent(SDL_SCANCODE_D, 0);
	latchedButtons = NativeInput_ConsumeKeyboard();
	if (((latchedButtons & 0x4000) != 0) || ((latchedButtons & 0x20) != 0))
	{
		fprintf(stderr, "[CTR Input] self-test failed: alias key-down tap was not latched\n");
		return 1;
	}
	if (!NativeInput_RunVirtualControllerSelfTest())
	{
		return 1;
	}

	printf("[CTR Input] self-test passed: metadata-key=%d legacy-enter=%d migration-enter=%d live-start=retail tap-latch=c+right one-snapshot aliases=12 held=k+d+e alias-tap=k+d virtual-gamepad=buttons+axes+rumble+hotplug\n",
	       SDL_SCANCODE_A, SDL_SCANCODE_RETURN, SDL_SCANCODE_RETURN);
	return 0;
}

int Platform_InputGetStateSize(void)
{
	return (int)sizeof(struct NativeInputStateSnapshot);
}

int Platform_InputCaptureState(void *dst, int dstSize)
{
	struct NativeInputStateSnapshot *snapshot = (struct NativeInputStateSnapshot *)dst;
	s32 slot;

	if ((dst == NULL) || (dstSize < (int)sizeof(*snapshot)))
	{
		return 0;
	}

	memset(snapshot, 0, sizeof(*snapshot));
	snapshot->magic = NATIVE_INPUT_STATE_MAGIC;
	snapshot->version = NATIVE_INPUT_STATE_VERSION;
	snapshot->size = sizeof(*snapshot);
	snapshot->keyboardControllerSlot = s_keyboardControllerSlot;
	snapshot->lastActiveControllerSlot = s_lastActiveControllerSlot;
	snapshot->installedSnapshotsActive = s_installedSnapshotsActive;

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		snapshot->installedSnapshots[slot] = s_installedSnapshots[slot];
		snapshot->controllers[slot].snapshot = s_controllers[slot].snapshot;
		snapshot->controllers[slot].analogEnabled = s_controllers[slot].analogEnabled;
		snapshot->controllers[slot].switchingAnalog = s_controllers[slot].switchingAnalog;
		snapshot->controllers[slot].controllerToSlotMapping = s_controllerToSlotMapping[slot];
	}

	return 1;
}

int Platform_InputRestoreState(const void *src, int srcSize)
{
	const struct NativeInputStateSnapshot *snapshot = (const struct NativeInputStateSnapshot *)src;
	s32 slot;

	if ((src == NULL) || (srcSize < (int)sizeof(*snapshot)))
	{
		return 0;
	}
	if ((snapshot->magic != NATIVE_INPUT_STATE_MAGIC) || (snapshot->version != NATIVE_INPUT_STATE_VERSION) || (snapshot->size != sizeof(*snapshot)))
	{
		return 0;
	}
	if (!NativeInput_IsValidControllerSlot(snapshot->keyboardControllerSlot))
	{
		return 0;
	}
	if ((snapshot->lastActiveControllerSlot < -1) || (snapshot->lastActiveControllerSlot >= NATIVE_INPUT_MAX_CONTROLLERS))
	{
		return 0;
	}
	if ((snapshot->installedSnapshotsActive < 0) || (snapshot->installedSnapshotsActive > 1))
	{
		return 0;
	}
	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		if ((snapshot->controllers[slot].analogEnabled < 0) || (snapshot->controllers[slot].analogEnabled > 1))
		{
			return 0;
		}
		if ((snapshot->controllers[slot].switchingAnalog < 0) || (snapshot->controllers[slot].switchingAnalog > 1))
		{
			return 0;
		}
		if (snapshot->controllers[slot].controllerToSlotMapping < -1)
		{
			return 0;
		}
	}

	s_keyboardControllerSlot = snapshot->keyboardControllerSlot;
	s_lastActiveControllerSlot = snapshot->lastActiveControllerSlot;
	s_installedSnapshotsActive = snapshot->installedSnapshotsActive != 0;

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		s_installedSnapshots[slot] = snapshot->installedSnapshots[slot];
		s_controllers[slot].snapshot = snapshot->controllers[slot].snapshot;
		s_controllers[slot].analogEnabled = snapshot->controllers[slot].analogEnabled;
		s_controllers[slot].switchingAnalog = snapshot->controllers[slot].switchingAnalog;
		s_controllerToSlotMapping[slot] = snapshot->controllers[slot].controllerToSlotMapping;
	}
	s_submitNameKey = NativeInput_GetSnapshotSubmitNameKey(&s_controllers[s_keyboardControllerSlot].snapshot);
	NativeInput_ClearKeyboardLatch();
	NativeInput_WritePadBus();

	return 1;
}

void Platform_InputPadVibrate(int port, unsigned char *table, int len)
{
	s32 physicalSlot = (port >> 4) & 1;
	s32 tap = port & 3;
	s32 slot;
	struct NativeInputController *controller;
	u16 freqHigh;
	u16 freqLow;

	if (NativeInput_UseMultitapBus() != 0)
	{
		if (physicalSlot != 0)
		{
			return;
		}
		slot = tap;
	}
	else
	{
		if (tap != 0)
		{
			return;
		}
		slot = physicalSlot;
	}

	if ((slot < 0) || (slot >= NATIVE_INPUT_MAX_CONTROLLERS) || (table == NULL) || (len <= 0))
	{
		return;
	}

	controller = &s_controllers[slot];
	if (controller->controller == NULL)
	{
		return;
	}

	freqHigh = table[0] * 255;
	freqLow = len > 1 ? table[1] * 255 : 0;

	if ((freqLow != 0) && (freqLow < 4096))
	{
		freqLow = 4096;
	}

	if ((freqHigh != 0) && (freqHigh < 4096))
	{
		freqHigh = 4096;
	}

	SDL_RumbleGamepad(controller->controller, freqLow, freqHigh, 200);
}

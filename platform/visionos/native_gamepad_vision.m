#import <GameController/GameController.h>

#include <platform/native_gamepad_vision.h>

#include <string.h>

static int16_t NativeVisionGamepad_Axis(float value)
{
	if (value <= -1.0f)
	{
		return INT16_MIN;
	}
	if (value >= 1.0f)
	{
		return INT16_MAX;
	}
	return (int16_t)(value * (float)INT16_MAX);
}

int NativeVisionGamepad_Read(struct NativeVisionGamepadState *state)
{
	if (state == NULL)
	{
		return 0;
	}

	memset(state, 0, sizeof(*state));
	@autoreleasepool
	{
		// For a single-player title, Apple's current controller is the one that
		// most recently received input. Fall back to enumeration before the first
		// button press, matching the proven Phosphor visionOS path.
		GCController *selectedController = GCController.current;
		if (selectedController.extendedGamepad == nil)
		{
			selectedController = nil;
			for (GCController *controller in GCController.controllers)
			{
				if (controller.extendedGamepad != nil)
				{
					selectedController = controller;
					break;
				}
			}
		}
		if (selectedController == nil)
		{
			return 0;
		}

		GCExtendedGamepad *gamepad = selectedController.extendedGamepad;
		uint16_t buttons = 0;

		if (gamepad.buttonX.isPressed) buttons |= 0x8000; /* Square */
		if (gamepad.buttonB.isPressed) buttons |= 0x2000; /* Circle */
		if (gamepad.buttonY.isPressed) buttons |= 0x1000; /* Triangle */
		if (gamepad.buttonA.isPressed) buttons |= 0x4000; /* Cross */
		if (gamepad.leftShoulder.isPressed) buttons |= 0x0400;
		if (gamepad.rightShoulder.isPressed) buttons |= 0x0800;
		if (gamepad.leftTrigger.isPressed) buttons |= 0x0100;
		if (gamepad.rightTrigger.isPressed) buttons |= 0x0200;
		if (gamepad.dpad.up.isPressed) buttons |= 0x0010;
		if (gamepad.dpad.down.isPressed) buttons |= 0x0040;
		if (gamepad.dpad.left.isPressed) buttons |= 0x0080;
		if (gamepad.dpad.right.isPressed) buttons |= 0x0020;
		if (gamepad.leftThumbstickButton.isPressed) buttons |= 0x0002;
		if (gamepad.rightThumbstickButton.isPressed) buttons |= 0x0004;
		if (gamepad.buttonOptions.isPressed) buttons |= 0x0001; /* Select */
		if (gamepad.buttonMenu.isPressed) buttons |= 0x0008; /* Start */

		state->pressedButtons = buttons;
		state->rightX = NativeVisionGamepad_Axis(gamepad.rightThumbstick.xAxis.value);
		state->rightY = NativeVisionGamepad_Axis(-gamepad.rightThumbstick.yAxis.value);
		state->leftX = NativeVisionGamepad_Axis(gamepad.leftThumbstick.xAxis.value);
		state->leftY = NativeVisionGamepad_Axis(-gamepad.leftThumbstick.yAxis.value);
	}
	return 1;
}

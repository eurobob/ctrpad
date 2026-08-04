#ifndef NATIVE_GAMEPAD_VISION_H
#define NATIVE_GAMEPAD_VISION_H

#include <stdint.h>

struct NativeVisionGamepadState
{
	uint16_t pressedButtons;
	int16_t rightX;
	int16_t rightY;
	int16_t leftX;
	int16_t leftY;
};

/* Reads the first connected extended GCController. Returns 1 when present. */
int NativeVisionGamepad_Read(struct NativeVisionGamepadState *state);

#endif

// Header
#include "gamepad.h"

typedef struct
{
	gamepadVmt_t vmt;
} gamepad_t;

void gamepadRead (void* pad)
{
	((gamepad_t*) pad)->vmt.read (pad);
}

gamepadButtonState_t gamepadGetButtonState (void* pad, gamepadButton_t button)
{
	return ((gamepad_t*) pad)->vmt.getButtonState (pad, button);
}

gamepadButtonEdge_t gamepadGetButtonEdge (void* pad, gamepadButton_t button)
{
	return ((gamepad_t*) pad)->vmt.getButtonEdge (pad, button);
}
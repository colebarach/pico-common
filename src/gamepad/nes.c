// Header
#include "nes.h"

// Includes -------------------------------------------------------------------------------------------------------------------

// RPi Pico SDK
#include <pico/stdlib.h>

// Constants ------------------------------------------------------------------------------------------------------------------

#define BUTTON_A		0b00000001
#define BUTTON_B		0b00000010
#define BUTTON_SELECT	0b00000100
#define BUTTON_START	0b00001000
#define BUTTON_UP		0b00010000
#define BUTTON_DOWN		0b00100000
#define BUTTON_LEFT		0b01000000
#define BUTTON_RIGHT	0b10000000

// Functions ------------------------------------------------------------------------------------------------------------------

static inline uint8_t getMask (gamepadButton_t button)
{
	switch (button)
	{
		case GAMEPAD_BUTTON_A:
			return BUTTON_A;
		case GAMEPAD_BUTTON_B:
			return  BUTTON_B;
		case GAMEPAD_BUTTON_SELECT:
			return BUTTON_SELECT;
		case GAMEPAD_BUTTON_START:
			return BUTTON_START;
		case GAMEPAD_BUTTON_UP:
			return BUTTON_UP;
		case GAMEPAD_BUTTON_DOWN:
			return BUTTON_DOWN;
		case GAMEPAD_BUTTON_LEFT:
			return BUTTON_LEFT;
		case GAMEPAD_BUTTON_RIGHT:
			return BUTTON_RIGHT;
		default:
			break;
	}

	return 0b00000000;
}

void nesInit (nes_t* pad, const nesConfig_t* config)
{
	// Init the struct
	*pad = (nes_t)
	{
		.vmt				=
		{
			.read			= nesRead,
			.getButtonState	= nesGetButtonState,
			.getButtonEdge	= nesGetButtonEdge
		},
		.config				= config,
		.buttonsPrevious	= 0b00000000,
		.buttonsCurrent		= 0b00000000
	};

	// Initialize GPIO

	gpio_init (config->pinSerialClock);
	gpio_set_dir (config->pinSerialClock, GPIO_OUT);

	gpio_init (config->pinSerialLatch);
	gpio_set_dir (config->pinSerialLatch, GPIO_OUT);

	gpio_init (config->pinSerialData);
	gpio_set_dir (config->pinSerialData, GPIO_IN);
	gpio_pull_up (config->pinSerialData);
}

void nesRead (void* pad)
{
	nes_t* nes = pad;
	uint32_t periodHalf = nes->config->clockPeriodUs / 2;

	nes->buttonsPrevious = nes->buttonsCurrent;
	nes->buttonsCurrent = 0b00000000;

	// Pulse the latch signal, read bit 0
	gpio_put (nes->config->pinSerialLatch, true);
	sleep_us (periodHalf);
	nes->buttonsCurrent |= !gpio_get (nes->config->pinSerialData);
	gpio_put (nes->config->pinSerialLatch, false);
	sleep_us (periodHalf);

	// Pulse the clock and read bits 1 to 7
	for (uint8_t index = 1; index < 8; ++index)
	{
		gpio_put (nes->config->pinSerialClock, true);
		sleep_us (periodHalf);
		nes->buttonsCurrent |= !(gpio_get (nes->config->pinSerialData)) << index;
		gpio_put (nes->config->pinSerialClock, false);
		sleep_us (periodHalf);
	}
}

gamepadButtonState_t nesGetButtonState (void* pad, gamepadButton_t button)
{
	nes_t* nes = pad;
	uint8_t mask = getMask (button);
	return ((nes->buttonsCurrent & mask) != 0) ? GAMEPAD_BUTTON_STATE_PRESSED : GAMEPAD_BUTTON_STATE_RELEASED;
}

gamepadButtonEdge_t nesGetButtonEdge (void* pad, gamepadButton_t button)
{
	nes_t* nes = pad;
	uint8_t mask = getMask (button);

	// If the current and previous states differ, return the current state
	if ((nes->buttonsCurrent ^ nes->buttonsPrevious) & mask)
		return ((nes->buttonsCurrent & mask) != 0) ? GAMEPAD_BUTTON_EDGE_PRESSED : GAMEPAD_BUTTON_EDGE_RELEASED;

	// Otherwise, no edge
	return GAMEPAD_BUTTON_EDGE_NONE;
}
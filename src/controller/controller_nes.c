// Header
#include "controller_nes.h"

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

void controllerNesInit (controllerNes_t* controller, const controllerNesConfig_t* config)
{
	// Init the struct
	*controller = (controllerNes_t)
	{
		.vmt =
		{
			.read			= controllerNesRead,
			.buttonPressed	= controllerNesButtonPressed,
			.buttonHeld		= controllerNesButtonHeld
		},
		.config = config
	};

	// Initialize GPIO

	gpio_init (controller->config->pinSerialClock);
	gpio_set_dir (controller->config->pinSerialClock, GPIO_OUT);

	gpio_init (controller->config->pinSerialLatch);
	gpio_set_dir (controller->config->pinSerialLatch, GPIO_OUT);

	gpio_init (controller->config->pinSerialData);
	gpio_set_dir (controller->config->pinSerialData, GPIO_IN);
	gpio_pull_up (controller->config->pinSerialData);
}

void controllerNesRead (void* controller)
{
	controllerNes_t* nes = controller;
	uint32_t periodHalf = nes->config->clockPeriodUs / 2;
	uint8_t pressed = 0x00;

	// Pulse the latch signal, read bit 0
	gpio_put (nes->config->pinSerialLatch, true);
	sleep_us (periodHalf);
	pressed |= !gpio_get (nes->config->pinSerialData);
	gpio_put (nes->config->pinSerialLatch, false);
	sleep_us (periodHalf);

	// Pulse the clock and read bits 1 to 7
	for (uint8_t index = 1; index < 8; ++index)
	{
		gpio_put (nes->config->pinSerialClock, true);
		sleep_us (periodHalf);
		pressed |= !(gpio_get (nes->config->pinSerialData)) << index;
		gpio_put (nes->config->pinSerialClock, false);
		sleep_us (periodHalf);
	}

	// Update button states
	nes->buttonsHeld = nes->buttonsPressed & pressed;
	nes->buttonsPressed = pressed & ~nes->buttonsHeld;
}

bool controllerNesButtonPressed (void* controller, controllerButton_t button)
{
	controllerNes_t* nes = controller;

	switch (button)
	{
		case CONTROLLER_BUTTON_A:
			return nes->buttonsPressed & BUTTON_A;
		case CONTROLLER_BUTTON_B:
			return nes->buttonsPressed & BUTTON_B;
		case CONTROLLER_BUTTON_SELECT:
			return nes->buttonsPressed & BUTTON_SELECT;
		case CONTROLLER_BUTTON_START:
			return nes->buttonsPressed & BUTTON_START;
		case CONTROLLER_BUTTON_UP:
			return nes->buttonsPressed & BUTTON_UP;
		case CONTROLLER_BUTTON_DOWN:
			return nes->buttonsPressed & BUTTON_DOWN;
		case CONTROLLER_BUTTON_LEFT:
			return nes->buttonsPressed & BUTTON_LEFT;
		case CONTROLLER_BUTTON_RIGHT:
			return nes->buttonsPressed & BUTTON_RIGHT;
		default:
			break;
	}

	return false;
}

bool controllerNesButtonHeld (void* controller, controllerButton_t button)
{
	controllerNes_t* nes = controller;

	switch (button)
	{
		case CONTROLLER_BUTTON_A:
			return nes->buttonsHeld & BUTTON_A;
		case CONTROLLER_BUTTON_B:
			return nes->buttonsHeld & BUTTON_B;
		case CONTROLLER_BUTTON_SELECT:
			return nes->buttonsHeld & BUTTON_SELECT;
		case CONTROLLER_BUTTON_START:
			return nes->buttonsHeld & BUTTON_START;
		case CONTROLLER_BUTTON_UP:
			return nes->buttonsHeld & BUTTON_UP;
		case CONTROLLER_BUTTON_DOWN:
			return nes->buttonsHeld & BUTTON_DOWN;
		case CONTROLLER_BUTTON_LEFT:
			return nes->buttonsHeld & BUTTON_LEFT;
		case CONTROLLER_BUTTON_RIGHT:
			return nes->buttonsHeld & BUTTON_RIGHT;
		default:
			break;
	}

	return false;
}
// Header
#include "controller_nes.h"

// Includes -------------------------------------------------------------------------------------------------------------------

// RPi Pico SDK
#include <pico/stdlib.h>

// Constants ------------------------------------------------------------------------------------------------------------------

#define CONTROLLER_NES_A		0b00000001
#define CONTROLLER_NES_B		0b00000010
#define CONTROLLER_NES_SELECT	0b00000100
#define CONTROLLER_NES_START	0b00001000
#define CONTROLLER_NES_UP		0b00010000
#define CONTROLLER_NES_DOWN		0b00100000
#define CONTROLLER_NES_LEFT		0b01000000
#define CONTROLLER_NES_RIGHT	0b10000000

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
	gpio_init (controller->config->pinSerialData);
	gpio_init (controller->config->pinSerialClock);
	gpio_init (controller->config->pinSerialLatch);
	gpio_set_dir (controller->config->pinSerialData,  GPIO_IN);
	gpio_set_dir (controller->config->pinSerialClock, GPIO_OUT);
	gpio_set_dir (controller->config->pinSerialLatch, GPIO_OUT);

	// Default to no input
	// - Pull-up resistor is not be required for a connected controller, but is for an unconnected controller.
	gpio_pull_up (controller->config->pinSerialData);
}

void controllerNesRead (void* controller)
{
	controllerNes_t* nes = controller;

	uint32_t clockPeriodHalf = nes->config->clockPeriod / 2;

	uint8_t pressed = 0x00;

	// Send latch signal, sample 'A' button
	gpio_put (nes->config->pinSerialLatch, true);
	sleep_us (clockPeriodHalf);
	pressed |= !gpio_get (nes->config->pinSerialData);
	gpio_put (nes->config->pinSerialLatch, false);
	sleep_us (clockPeriodHalf);

	// Read 'B' button
	gpio_put (nes->config->pinSerialClock, true);
	sleep_us (clockPeriodHalf);
	pressed |= !gpio_get (nes->config->pinSerialData) << 1;
	gpio_put (nes->config->pinSerialClock, false);
	sleep_us (clockPeriodHalf);

	// Get select button
	gpio_put (nes->config->pinSerialClock, true);
	sleep_us (clockPeriodHalf);
	pressed |= !gpio_get (nes->config->pinSerialData) << 2;
	gpio_put (nes->config->pinSerialClock, false);
	sleep_us (clockPeriodHalf);

	// Get start button
	gpio_put (nes->config->pinSerialClock, true);
	sleep_us (clockPeriodHalf);
	pressed |= !gpio_get (nes->config->pinSerialData) << 3;
	gpio_put (nes->config->pinSerialClock, false);
	sleep_us (clockPeriodHalf);

	// Get up button
	gpio_put (nes->config->pinSerialClock, true);
	sleep_us (clockPeriodHalf);
	pressed |= !gpio_get (nes->config->pinSerialData) << 4;
	gpio_put (nes->config->pinSerialClock, false);
	sleep_us (clockPeriodHalf);

	// Get down button
	gpio_put (nes->config->pinSerialClock, true);
	sleep_us (clockPeriodHalf);
	pressed |= !gpio_get (nes->config->pinSerialData) << 5;
	gpio_put (nes->config->pinSerialClock, false);
	sleep_us (clockPeriodHalf);

	// Get left button
	gpio_put (nes->config->pinSerialClock, true);
	sleep_us (clockPeriodHalf);
	pressed |= !gpio_get (nes->config->pinSerialData) << 6;
	gpio_put (nes->config->pinSerialClock, false);
	sleep_us (clockPeriodHalf);

	// Get right button
	gpio_put (nes->config->pinSerialClock, true);
	sleep_us (clockPeriodHalf);
	pressed |= !gpio_get (nes->config->pinSerialData) << 7;
	gpio_put (nes->config->pinSerialClock, false);
	sleep_us (clockPeriodHalf);

	nes->buttonsHeld = nes->buttonsPressed & pressed;
	nes->buttonsPressed = pressed & ~nes->buttonsHeld;
}

bool controllerNesButtonPressed (void* controller, controllerButton_t button)
{
	controllerNes_t* nes = controller;

	switch (button)
	{
		case CONTROLLER_BUTTON_A:
			return nes->buttonsPressed & CONTROLLER_NES_A;
		case CONTROLLER_BUTTON_B:
			return nes->buttonsPressed & CONTROLLER_NES_B;
		case CONTROLLER_BUTTON_SELECT:
			return nes->buttonsPressed & CONTROLLER_NES_SELECT;
		case CONTROLLER_BUTTON_START:
			return nes->buttonsPressed & CONTROLLER_NES_START;
		case CONTROLLER_BUTTON_UP:
			return nes->buttonsPressed & CONTROLLER_NES_UP;
		case CONTROLLER_BUTTON_DOWN:
			return nes->buttonsPressed & CONTROLLER_NES_DOWN;
		case CONTROLLER_BUTTON_LEFT:
			return nes->buttonsPressed & CONTROLLER_NES_LEFT;
		case CONTROLLER_BUTTON_RIGHT:
			return nes->buttonsPressed & CONTROLLER_NES_RIGHT;
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
			return nes->buttonsHeld & CONTROLLER_NES_A;
		case CONTROLLER_BUTTON_B:
			return nes->buttonsHeld & CONTROLLER_NES_B;
		case CONTROLLER_BUTTON_SELECT:
			return nes->buttonsHeld & CONTROLLER_NES_SELECT;
		case CONTROLLER_BUTTON_START:
			return nes->buttonsHeld & CONTROLLER_NES_START;
		case CONTROLLER_BUTTON_UP:
			return nes->buttonsHeld & CONTROLLER_NES_UP;
		case CONTROLLER_BUTTON_DOWN:
			return nes->buttonsHeld & CONTROLLER_NES_DOWN;
		case CONTROLLER_BUTTON_LEFT:
			return nes->buttonsHeld & CONTROLLER_NES_LEFT;
		case CONTROLLER_BUTTON_RIGHT:
			return nes->buttonsHeld & CONTROLLER_NES_RIGHT;
		default:
			break;
	}

	return false;
}
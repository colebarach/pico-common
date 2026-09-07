#ifndef CONTROLLER_H
#define CONTROLLER_H

// TODO(Barach): Needs base type for casting, plus strict aliasing needs disabled for compilation

// C Standard Library
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
	CONTROLLER_BUTTON_A,
	CONTROLLER_BUTTON_B,
	CONTROLLER_BUTTON_X,
	CONTROLLER_BUTTON_Y,
	CONTROLLER_BUTTON_L,
	CONTROLLER_BUTTON_R,
	CONTROLLER_BUTTON_UP,
	CONTROLLER_BUTTON_DOWN,
	CONTROLLER_BUTTON_LEFT,
	CONTROLLER_BUTTON_RIGHT,
	CONTROLLER_BUTTON_START,
	CONTROLLER_BUTTON_SELECT
} controllerButton_t;

typedef void (controllerRead_t) (void* controller);

typedef bool (controllerButtonPressed_t) (void* controller, controllerButton_t button);

typedef bool (controllerButtonHeld_t) (void* controller, controllerButton_t button);

typedef struct
{
	controllerRead_t*			read;
	controllerButtonPressed_t*	buttonPressed;
	controllerButtonHeld_t*		buttonHeld;
} controllerVmt_t;

#define controllerRead(controller) ((controller)->vmt.read (controller))

#define controllerButtonPressed(controller, button) ((controller)->vmt.buttonPressed (controller, button))

#define controllerButtonHeld(controller, button) ((controller)->vmt.buttonHeld (controller, button))

#endif // CONTROLLER_H
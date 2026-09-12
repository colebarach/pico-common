#ifndef GAMEPAD_H
#define GAMEPAD_H

// NOTE: If linking against this module, strict aliasing must be disabled.

// TODO(Barach):
// - Debouncing?

// Includes -------------------------------------------------------------------------------------------------------------------

// C Standard Library
#include <stdbool.h>
#include <stdint.h>

// Datatypes ------------------------------------------------------------------------------------------------------------------

typedef enum
{
	GAMEPAD_BUTTON_A,
	GAMEPAD_BUTTON_B,
	GAMEPAD_BUTTON_X,
	GAMEPAD_BUTTON_Y,
	GAMEPAD_BUTTON_L,
	GAMEPAD_BUTTON_R,
	GAMEPAD_BUTTON_UP,
	GAMEPAD_BUTTON_DOWN,
	GAMEPAD_BUTTON_LEFT,
	GAMEPAD_BUTTON_RIGHT,
	GAMEPAD_BUTTON_START,
	GAMEPAD_BUTTON_SELECT
} gamepadButton_t;

typedef enum
{
	/// @brief Indicates a button is not being pressed.
	GAMEPAD_BUTTON_STATE_RELEASED = 0,
	/// @brief Indicates a button is being pressed.
	GAMEPAD_BUTTON_STATE_PRESSED = 1,
} gamepadButtonState_t;

typedef enum
{
	/// @brief Indicates a button has not changed states.
	GAMEPAD_BUTTON_EDGE_NONE = -1,
	/// @brief Indicates the button was previously pressed and is now released.
	GAMEPAD_BUTTON_EDGE_RELEASED = 0,
	/// @brief Indicates the button was previously released and is now pressed.
	GAMEPAD_BUTTON_EDGE_PRESSED = 1
} gamepadButtonEdge_t;

/// @brief Function signature for @c gamepadRead .
typedef void (gamepadRead_t) (void* pad);

/// @brief Function signature for @c gamepadGetButtonState .
typedef gamepadButtonState_t (gamepadGetButtonState_t) (void* pad, gamepadButton_t button);

/// @brief Function signature for @c gamepadGetButtonEdge .
typedef gamepadButtonEdge_t (gamepadGetButtonEdge_t) (void* pad, gamepadButton_t button);

/// @brief Virtual method table for a @c gamepad .
typedef struct
{
	gamepadRead_t*				read;
	gamepadGetButtonState_t*	getButtonState;
	gamepadGetButtonEdge_t*		getButtonEdge;
} gamepadVmt_t;

// Functions ------------------------------------------------------------------------------------------------------------------

/**
 * @brief Reads all inputs from a gamepad.
 * @note This does not implement debouncing, rather, the gamepad is expected to be polled at a reasonable rate.
 * @param pad The gamepad to read from.
 */
void gamepadRead (void* pad);

/**
 * @brief Gets the state of a gamepad button. Button states are updated once per call to @c gamepadRead .
 * @param pad The gamepad to read from.
 * @param button The button to check.
 * @return The state of the button. @c GAMEPAD_BUTTON_STATE_RELEASED if the controller does not map the button.
 */
gamepadButtonState_t gamepadGetButtonState (void* pad, gamepadButton_t button);

/**
 * @brief Gets the rising/galling edge of a gamepad button. Edge transitions are updated once per call to @c gamepadRead .
 * @param pad The gamepad to read from.
 * @param button The button to check.
 * @return The rising/falling edge of the button. @c GAMEPAD_BUTTON_EDGE_NONE if the controller does not map the button.
 */
gamepadButtonEdge_t gamepadGetButtonEdge (void* pad, gamepadButton_t button);

#endif // GAMEPAD_H
// MCP2515 Device Driver ------------------------------------------------------------------------------------------------------
//
// Author: Cole Barach
// Date Created: 2026.09.04
//
// Description: Device driver for the MCP2515 CAN controller.

// TODO(Barach):
// - Input validation (overflow)
// - Implemente the ANY buffers

// Includes -------------------------------------------------------------------------------------------------------------------

// RPi Pico SDK
#include "hardware/spi.h"
#include "pico/mutex.h"

// Constants ------------------------------------------------------------------------------------------------------------------

// TODO(Barach): Not implemented
#define MCP2515_TIMEOUT_NONBLOCKING		((unsigned int) 0)
#define MCP2515_TIMEOUT_BLOCKING		((unsigned int) -1)

// Datatypes ------------------------------------------------------------------------------------------------------------------

typedef struct
{
	/// @brief The ID of the CAN message, may be either an SID or an EID (extended identifier), as determined by the @c ide
	/// field.
	uint32_t id;

	/// @brief The IDE bit of the message. If @c false , @c id is a SID, if @c true , @c id is an EID.
	bool ide;

	/// @brief The RTR (remote transmission request) bit of the message.
	bool rtr;

	/// @brief The DLC (data length code) of the message.
	uint8_t dlc;

	/// @brief The payload of the message. Note that only @c dlc elements are used.
	uint8_t data [8];
} mcp2515CanFrame_t;

// TODO(Barach): Does this even need exposed?
typedef enum
{
	MCP2515_TX_BUFFER_0 = 0,
	MCP2515_TX_BUFFER_1 = 1,
	MCP2515_TX_BUFFER_2 = 2,
	MCP2515_TX_BUFFER_ANY = 3
} mcp2515TxIndex_t;

typedef enum
{
	MCP2515_RX_BUFFER_0 = 0,
	MCP2515_RX_BUFFER_1 = 1,
	MCP2515_RX_BUFFER_ANY = 2
} mcp2515RxIndex_t;

typedef enum
{
	MCP2515_SUCCESS = 0,
	MCP2515_ERROR_CONFIG,
	MCP2515_ERROR_TIMEOUT,
	MCP2515_ERROR_LOST_ARB,
	MCP2515_ERROR_BUS_ERROR,
	MCP2515_ERROR_NONBLOCKING,
	MCP2515_ERROR_UNKNOWN
} mcp2515Error_t;

typedef struct
{
	spi_inst_t* spi;
	mutex_t* spiMutex;
	unsigned int chipSelectLine;
	unsigned int interruptLine;
	unsigned int frequency;
} mcp2515Config_t;

typedef struct
{
	const mcp2515Config_t* config;
	mutex_t canintfMutex;
	bool canintfPolling;
	uint8_t canintf;
} mcp2515_t;

// Functions ------------------------------------------------------------------------------------------------------------------

/**
 * @brief Initializes the driver for an MCP2515 CAN controller.
 * @param mcp The device to initialize.
 * @param config The configuration to use.
 * @return @c MCP2515_SUCCESS if successful, the error code otherwise.
 */
mcp2515Error_t mcp2515Init (mcp2515_t* mcp, const mcp2515Config_t* config);

/**
 * @brief Transmits a CAN frame.
 * @param mcp The device to use.
 * @param frame The frame to transmit.
 * @param index The TX buffer to use.
 * @param timeoutUs The amount of time to timeout after, in microseconds.
 * @return @c MCP2515_SUCCESS if successful, the error code otherwise.
 */
mcp2515Error_t mcp2515Transmit (mcp2515_t* mcp, mcp2515CanFrame_t* frame, mcp2515TxIndex_t index, unsigned int timeoutUs);

/**
 * @brief Receive a CAN frame.
 * @param mcp The device to use.
 * @param frame Buffer to write the received frame into.
 * @param index The RX buffer to receive from.
 * @param timeoutUs The amount of time to timeout after, in microseconds.
 * @return @c MCP2515_SUCCESS if successful, the error code otherwise.
 */
mcp2515Error_t mcp2515Receive (mcp2515_t* mcp, mcp2515CanFrame_t* frame, mcp2515RxIndex_t index, unsigned int timeoutUs);

/**
 * @brief Converts a MCP2515 error into a string.
 * @param error The error to convert.
 * @return A statically-allocated string describing the error.
 */
char* mcp2515Strerror (mcp2515Error_t error);
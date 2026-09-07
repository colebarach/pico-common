// Header
#include "mcp2515.h"

// RPi Pico SDK
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// C Standard Library
#include <string.h>

// TODO(Barach): Disable interrupts we aren't looking for dynamically

// Constants ------------------------------------------------------------------------------------------------------------------

// Instructions (page 65)
#define INST_RESET					0b11000000
#define INST_READ					0b00000011
#define INST_WRITE					0b00000010
#define INST_RTS(n)					(0b10000000 | (1 << (n)))
#define INST_BIT_MODIFY				0b00000101
#define INST_READ_RX_BUFFER(n, m)	(0b10010000 | ((n) << 2) | ((m) << 1))
#define INST_LOAD_TX_BUFFER(n, m)	(0b01000000 | ((n) << 1) | (m))

// Register Map (page 63)
#define ADDR_CANCTRL				0x0F
#define ADDR_CANSTAT				0x0E
#define ADDR_CNF1					0x2A
#define ADDR_CNF2					0x29
#define ADDR_CNF3					0x28
#define ADDR_CANINTE				0x2B
#define ADDR_CANINTF				0x2C
#define ADDR_EFLG					0x2D
#define ADDR_TXBnCTRL(n)			(0x30 + 0x10*(n))
#define ADDR_TXBnSIDH(n)			(0x31 + 0x10*(n))
#define ADDR_TXBnSIDL(n)			(0x32 + 0x10*(n))
#define ADDR_TXBnEID8(n)			(0x33 + 0x10*(n))
#define ADDR_TXBnEID0(n)			(0x34 + 0x10*(n))
#define ADDR_TXBnDLC(n)				(0x35 + 0x10*(n))
#define ADDR_TXBnDm(n, m)			(0x36 + 0x10*(n) + (m))
#define ADDR_RXB0CTRL				0x60
#define ADDR_RXB1CTRL				0x70
#define ADDR_RXBnSIDH(n)			(0x61 + 0x10*(n))
#define ADDR_RXBnSIDL(n)			(0x62 + 0x10*(n))
#define ADDR_RXBnEID8(n)			(0x63 + 0x10*(n))
#define ADDR_RXBnEID0(n)			(0x64 + 0x10*(n))
#define ADDR_RXBnDLC(n)				(0x65 + 0x10*(n))
#define ADDR_RXBnDm(n, m)			(0x66 + 0x10*(n) + (m))

// CANCTRL Register (page 60)
#define CANCTRL_REQOP_NORMAL		(0b000 << 5)
#define CANCTRL_REQOP_SLEEP			(0b001 << 5)
#define CANCTRL_REQOP_LOOPBACK		(0b010 << 5)
#define CANCTRL_REQOP_LISTEN_ONLY	(0b011 << 5)
#define CANCTRL_REQOP_CONFIG		(0b100 << 5)
#define CANCTRL_ABAT				(1 << 4)
#define CANCTRL_OSM					(1 << 3)
#define CANCTRL_CLKEN				(1 << 2)
#define CANCTRL_CLKPRE(pre)			((pre) << 0)

// CANSTAT Register (page 61)
#define CANSTAT_OPMOD(canstat)		((canstat) >> 5)
#define CANSTAT_ICOD(canstat)		(((canstat) >> 1) & 0b111)
#define CANSTAT_OPMOD_NORMAL		0b000
#define CANCTRL_OPMOD_SLEEP			0b001
#define CANCTRL_OPMOD_LOOPBACK		0b010
#define CANCTRL_OPMOD_LISTEN_ONLY	0b011
#define CANCTRL_OPMOD_CONFIG		0b100

// CNF1 Register (page 44)
#define CNF1_SJW(sjw)				((sjw) << 6)
#define CNF1_BRP(brp)				((brp) << 0)

// CNF2 Register (page 44)
#define CNF2_PRSEG(prseg)			((prseg) << 0)
#define CNF2_PHSEG1(phseg1)			((phseg1) << 3)
#define CNF2_SAM					(1 << 6)
#define CNF2_BTLMODE				(1 << 7)

// CNF3 Register (page 45)
#define CNF3_PHSEG2(phseg2)			((phseg2) << 0)
#define CNF3_WAKFIL					(1 << 6)
#define CNF3_SOF					(1 << 7)

// CANINTE Register (page 53)
#define CANINTE_MERRE				(1 << 7)
#define CANINTE_WAKIE				(1 << 6)
#define CANINTE_ERRIE				(1 << 5)
#define CANINTE_TXnIE(n)			(1 << ((n) + 2))
#define CANINTE_RXnIE(n)			(1 << ((n) + 0))

// CANINTF Register (page 54)
#define CANINTF_MERRF				(1 << 7)
#define CANINTF_WAKIF				(1 << 6)
#define CANINTF_ERRIF				(1 << 5)
#define CANINTF_TXnIF(n)			(1 << ((n) + 2))
#define CANINTF_RXnIF(n)			(1 << ((n) + 0))

// TXBnCTRL Registers (page 18)
#define TXBnCTRL_ABTF				(1 << 6)
#define TXBnCTRL_MLOA				(1 << 5)
#define TXBnCTRL_TXERR				(1 << 4)
#define TXBnCTRL_TXREQ				(1 << 3)
#define TXBnCTRL_TXP(txbnctrl)		((txbnctrl) & 0b11)

// TXBnSIDH Registers (page 20)
#define TXBnSIDH_SID(sid)			((sid) >> 3)
#define TXBnSIDH_EID(eid)			((eid) >> 21)

// TXBnSIDL Registers (page 20)
#define TXBnSIDL_SID(sid)			((sid) << 5)
#define TXBnSIDL_EXIDE				(1 << 3)
#define TXBnSIDL_EID(eid)			((((eid) >> 16) & 0b11) | ((eid >> 13) & 0b11100000))

// TXBnEID8 Registers (page 21)
#define TXBnEID8_EID(eid)			((eid) >> 8)

// TXBnEID0 Registers (page 21)
#define TXBnEID0_EID(eid)			((eid) >> 0)

// TXBnDLC Registers (page 22)
#define TXBnDLC_RTR					(1 << 6)
#define TXBnDLC_DLC(dlc)			(dlc)

// RXB0CTRL Register (page 27)
#define RXB0CTRL_RXM_ALL			(0b11 << 5)
#define RXB0CTRL_RXRTR				(1 << 3)
#define RXB0CTRL_BUKT				(1 << 2)
#define RXB0CTRL_FILHIT0			(1 << 0)

// RXB1CTRL Register (page 28)
#define RXB1CTRL_RXM_ALL			(0b11 << 5)
#define RXB1CTRL_RXRTR				(1 << 3)
#define RXB1CTRL_FILHIT(rxb1ctrl)	(((rxb1ctrl) >> 0) & 0b111)

// RXBnSIDH Registers (page 30)
#define RXBnSIDH_SID(rxbnsidh)		((rxbnsidh) << 3)
#define RXBnSIDH_EID(rxbnsidh)		((rxbnsidh) << 21)

// RXBnSIDL Registers (page 30)
#define RXBnSIDL_SID(rxbnsidl)		((rxbnsidl) >> 5)
#define RXBnSIDL_SRR				(1 << 4)
#define RXBnSIDL_IDE				(1 << 3)
#define RXBnSIDL_EID(rxbnsidl)		((((rxbnsidl) & 0b11) << 16) | (((rxbnsidl) & 0b11100000) << 13))

// RXBnEID8 Registers (page 31)
#define RXBnEID8_EID(rxbneid8)		((rxbneid8) << 8)

// RXBnEID0 Registers (page 31)
#define RXBnEID0_EID(rxbneid0)		((rxbneid0) << 0)

// RXBnDLC Registers (page 22)
#define RXBnDLC_RTR					(1 << 6)
#define RXBnDLC_DLC(rxbndlc)		(rxbndlc & 0b1111)

// Datatypes ------------------------------------------------------------------------------------------------------------------

typedef struct
{
	// First SPI transaction
	uint8_t inst0;
	uint8_t sidh;
	uint8_t sidl;
	uint8_t eid8;
	uint8_t eid0;
	uint8_t dlc;

	// Second SPI transaction
	uint8_t inst1;
	uint8_t d [8];
} rxbn_t;

typedef struct
{
	uint8_t inst;
	uint8_t sidh;
	uint8_t sidl;
	uint8_t eid8;
	uint8_t eid0;
	uint8_t dlc;
	uint8_t d [8];
} txbn_t;

// Functions ------------------------------------------------------------------------------------------------------------------

static inline void spiLock (mcp2515_t* mcp)
{
	mutex_enter_blocking (mcp->config->spiMutex);
}

static inline bool spiLockTimeout (mcp2515_t* mcp, absolute_time_t timeout)
{
	return mutex_enter_block_until (mcp->config->spiMutex, timeout);
}

static inline void spiUnlock (mcp2515_t* mcp)
{
	mutex_exit (mcp->config->spiMutex);
}

static inline void spiSelect (mcp2515_t* mcp)
{
	gpio_put (mcp->config->chipSelectLine, false);
}

static inline void spiDeselect (mcp2515_t* mcp)
{
	gpio_put (mcp->config->chipSelectLine, true);
}

static void instReset (mcp2515_t* mcp)
{
	uint8_t txBuffer [] = { INST_RESET };
	spiSelect (mcp);
	spi_write_blocking (mcp->config->spi, txBuffer, sizeof (txBuffer));
	spiDeselect (mcp);

	// Wait until device has left reset state (128 clock cycles)
	sleep_us ((128 * 1e6 / mcp->config->frequency) + 1);
}

static void instWrite (mcp2515_t* mcp, uint8_t addr, uint8_t data)
{
	uint8_t txBuffer [] = { INST_WRITE, addr, data };

	spiSelect (mcp);
	spi_write_blocking (mcp->config->spi, txBuffer, sizeof (txBuffer));
	spiDeselect (mcp);
}

static uint8_t instRead (mcp2515_t* mcp, uint8_t addr)
{
	uint8_t txBuffer [] = { INST_READ, addr, 0x00 };
	uint8_t rxBuffer [sizeof (txBuffer)];

	spiSelect (mcp);
	spi_write_read_blocking (mcp->config->spi, txBuffer, rxBuffer, sizeof (txBuffer));
	spiDeselect (mcp);

	return rxBuffer [2];
}

static void instReadRxBuffer (mcp2515_t* mcp, mcp2515RxIndex_t index, rxbn_t* rxbn)
{
	rxbn_t src =
	{
		.inst0 = INST_READ_RX_BUFFER (index, 0),
		.inst1 = INST_READ_RX_BUFFER (index, 1)
	};

	// First transaction
	spiSelect (mcp);
	spi_write_read_blocking (mcp->config->spi, &src.inst0, &rxbn->inst0, offsetof (rxbn_t, inst1));
	spiDeselect (mcp);

	// Extract DLC
	uint8_t dlc = RXBnDLC_DLC (rxbn->dlc);

	// Second transaction
	spiSelect (mcp);
	spi_write_read_blocking (mcp->config->spi, &src.inst1, &rxbn->inst1, 1 + dlc);
	spiDeselect (mcp);
}

static void instLoadTxBuffer (mcp2515_t* mcp, mcp2515TxIndex_t index, txbn_t* txbn)
{
	// Set the instruction
	txbn->inst = INST_LOAD_TX_BUFFER (index, 0);

	uint8_t dlc = TXBnDLC_DLC (txbn->dlc);

	spiSelect (mcp);
	spi_write_blocking (mcp->config->spi, &txbn->inst, offsetof (txbn_t, d) + dlc);
	spiDeselect (mcp);
}

static void instBitModify (mcp2515_t* mcp, uint8_t addr, uint8_t mask, uint8_t data)
{
	// Bit Modify Instruction (page 69)
	uint8_t txBuffer [] = {INST_BIT_MODIFY, addr, mask, data };

	spiSelect (mcp);
	spi_write_blocking (mcp->config->spi, txBuffer, sizeof (txBuffer));
	spiDeselect (mcp);
}

static void instRequestToSend (mcp2515_t* mcp, mcp2515TxIndex_t index)
{
	// RTS Instruction (page 69)
	uint8_t txBuffer [] = { INST_RTS (index) };

	spiSelect (mcp);
	spi_write_blocking (mcp->config->spi, txBuffer, sizeof (txBuffer));
	spiDeselect (mcp);
}

/**
 * @brief Waits for an interrupt to flag in the CANINTF register.
 * @note The SPI peripheral should be locked on entrance and will be locked upon exit.
 * @param mcp The device to wait for.
 * @param mask Mask of flags to wait for. The operation will return when at least 1 of these flags are asserted.
 * @param timeout The time the operation should timeout at.
 * @return The read flags. If none of the flags in @c mask are asserted, the operation timed out.
 */
static uint8_t waitForInterrupt (mcp2515_t* mcp, uint8_t mask, absolute_time_t timeout)
{
	// Enable the specified interrupts
	instBitModify (mcp, ADDR_CANINTE, mask, 0xFF);
	spiUnlock (mcp);

	uint8_t canintf;
	do
	{
		mutex_enter_blocking (&mcp->canintfMutex);

		// Check whether the interrupt is already being polled by another core.
		if (!mcp->canintfPolling)
		{
			// If not, this core will poll for interrupts
			// - Default to no flags in case of timeout
			mcp->canintfPolling = true;
			mcp->canintf = 0x00;

			// Poll until an expected flag is read or a timeout occurs
			do
			{
				// Wait for the interrupt line to read low
				if (gpio_get (mcp->config->interruptLine))
					continue;

				// Read the interrupt flags
				if (!spiLockTimeout (mcp, timeout))
					break;
				mcp->canintf = instRead (mcp, ADDR_CANINTF);
				spiUnlock (mcp);

				// Allow other cores to read the interrupt flags, if they are waiting
				mutex_exit (&mcp->canintfMutex);
				mutex_enter_blocking (&mcp->canintfMutex);

				// If an expected flag was read, exit
				if ((mcp->canintf & mask) != 0)
					break;
			} while (absolute_time_diff_us (get_absolute_time (), timeout) > 0);
			mcp->canintfPolling = false;
		}

		// Create a local copy of the flags while we are still inside the mutex
		canintf = mcp->canintf;

		mutex_exit (&mcp->canintfMutex);

		// If an expected flag was read (by either this core or another), exit
		if ((canintf & mask) != 0)
			break;
	} while (absolute_time_diff_us (get_absolute_time (), timeout) > 0);

	// Disable the specified interrupts
	// - This is a blocking lock, as we must disable interrupts. If we have timed out already, the operation will still fail
	//   as expected.
	spiLock (mcp);
	instBitModify (mcp, ADDR_CANINTE, mask, 0x00);

	// Return the read flags
	return canintf;
}

mcp2515Error_t mcp2515Init (mcp2515_t* mcp, const mcp2515Config_t* config)
{
	// Init the struct
	*mcp = (mcp2515_t)
	{
		.config = config,
		.canintfPolling = false
	};
	mutex_init (&mcp->canintfMutex);

	// Lock the SPI peripheral
	spiLock (mcp);

	// Reset the device
	instReset (mcp);

	// Validate the device entered config mode
	uint8_t canstat = instRead (mcp, ADDR_CANSTAT);
	if (CANSTAT_OPMOD (canstat) != CANCTRL_OPMOD_CONFIG)
	{
		// Device did not enter config mode
		spiUnlock (mcp);
		return MCP2515_ERROR_CONFIG;
	}

	// Write bit time configuration
	// TODO(Barach): Extract to config
	instWrite (mcp, ADDR_CNF1, CNF1_SJW (1) | CNF1_BRP (4));
	instWrite (mcp, ADDR_CNF2, CNF2_BTLMODE | CNF2_SAM | CNF2_PHSEG1 (4) | CNF2_PRSEG (0));
	instWrite (mcp, ADDR_CNF3, CNF3_PHSEG2 (0));

	// Disable message filtering, use RXB1 for overflow
	instWrite (mcp, ADDR_RXB0CTRL, RXB0CTRL_RXM_ALL | RXB0CTRL_BUKT);
	instWrite (mcp, ADDR_RXB1CTRL, RXB1CTRL_RXM_ALL);

	// Enter normal operation mode
	instWrite (mcp, ADDR_CANCTRL, CANCTRL_REQOP_NORMAL);

	// Success
	spiUnlock (mcp);
	return MCP2515_SUCCESS;
}

mcp2515Error_t mcp2515Transmit (mcp2515_t* mcp, mcp2515CanFrame_t* frame, mcp2515TxIndex_t index, unsigned timeoutUs)
{
	// TODO(Barach): How to do blocking / non-blocking
	absolute_time_t timeout = make_timeout_time_us (timeoutUs);
	if (!spiLockTimeout (mcp, timeout))
		return MCP2515_ERROR_TIMEOUT;

	// Populate the registers
	txbn_t txbn =
	{
		.dlc = TXBnDLC_DLC (frame->dlc) | (frame->rtr ? TXBnDLC_RTR : 0)
	};
	if (frame->ide)
	{
		// Extended ID
		txbn.sidh = TXBnSIDH_EID (frame->id);
		txbn.sidl = TXBnSIDL_EID (frame->id) | TXBnSIDL_EXIDE;
		txbn.eid8 = TXBnEID8_EID (frame->id);
		txbn.eid0 = TXBnEID0_EID (frame->id);
	}
	else
	{
		// Standard ID
		txbn.sidh = TXBnSIDH_SID (frame->id);
		txbn.sidl = TXBnSIDL_SID (frame->id);
	}
	memcpy (txbn.d, frame->data, frame->dlc);

	// Write to the TX registers
	instLoadTxBuffer (mcp, index, &txbn);

	// Transmit the message
	instRequestToSend (mcp, index);

	uint8_t canintf = waitForInterrupt (mcp, CANINTF_TXnIF (index), timeout);
	if ((canintf & CANINTF_TXnIF (index)) == 0)
	{
		// Transmit failed, handle the error

		// Read the TXBnCTRL register to determine the error
		uint8_t txbnctrl = instRead (mcp, ADDR_TXBnCTRL (index));

		// Abort the transmission then clear the interrupt
		instWrite (mcp, ADDR_TXBnCTRL (index), 0x00);
		instBitModify (mcp, ADDR_CANINTF, CANINTF_TXnIF (index) | CANINTF_MERRF, 0x00);

		spiUnlock (mcp);

		if (txbnctrl & TXBnCTRL_MLOA)
			return MCP2515_ERROR_LOST_ARB;
		if (txbnctrl & TXBnCTRL_TXERR)
			return MCP2515_ERROR_BUS_ERROR;

		return MCP2515_ERROR_TIMEOUT;
	}

	// Clear the interrupt
	instBitModify (mcp, ADDR_CANINTF, CANINTF_TXnIF (index) | CANINTF_MERRF, 0x00);

	// Success
	spiUnlock (mcp);
	return MCP2515_SUCCESS;
}

mcp2515Error_t mcp2515Receive (mcp2515_t* mcp, mcp2515CanFrame_t* frame, mcp2515RxIndex_t index, unsigned timeoutUs)
{
	// TODO(Barach): Blocking / non-blocking
	absolute_time_t timeout = make_timeout_time_us (timeoutUs);
	if (!spiLockTimeout (mcp, timeout))
		return MCP2515_ERROR_TIMEOUT;

	// TODO(Barach): Handle errors.

	// Wait for a message to be received. If no message was received, return timeout.
	uint8_t canintf = waitForInterrupt (mcp, CANINTF_RXnIF (index), timeout);
	if ((canintf & CANINTF_RXnIF (index)) == 0)
	{
		spiUnlock (mcp);
		return MCP2515_ERROR_TIMEOUT;
	}

	// Read the received message
	rxbn_t rxbn;
	instReadRxBuffer (mcp, index, &rxbn);

	// Reset the interrupt
	instBitModify (mcp, ADDR_CANINTF, CANINTF_RXnIF (index), 0x00);

	// Parse the registers
	frame->ide = rxbn.sidl & RXBnSIDL_IDE;
	if (frame->ide)
	{
		frame->id = RXBnSIDH_EID (rxbn.sidh) | RXBnSIDL_EID (rxbn.sidl)
			| RXBnEID8_EID (rxbn.eid8) | RXBnEID0_EID (rxbn.eid0);
		frame->rtr = rxbn.dlc & RXBnDLC_RTR;
	}
	else
	{
		frame->id = RXBnSIDH_SID (rxbn.sidh) | RXBnSIDL_SID (rxbn.sidl);
		frame->rtr = rxbn.sidl & RXBnSIDL_SRR;
	}
	frame->dlc = RXBnDLC_DLC (rxbn.dlc);
	memcpy (frame->data, rxbn.d, frame->dlc);

	// Success
	spiUnlock (mcp);
	return MCP2515_SUCCESS;
}

char* mcp2515Strerror (mcp2515Error_t error)
{
	switch (error)
	{
	case MCP2515_SUCCESS:
		return "Success";
	case MCP2515_ERROR_CONFIG:
		return "Device did not enter config mode";
	case MCP2515_ERROR_TIMEOUT:
		return "Operation has timed out";
	case MCP2515_ERROR_LOST_ARB:
		return "Lost arbitration on CAN bus";
	case MCP2515_ERROR_BUS_ERROR:
		return "An error occurred on the CAN bus";
	case MCP2515_ERROR_NONBLOCKING:
		return "Non-blocking specified for a blocking operation";
	case MCP2515_ERROR_UNKNOWN:
		break;
	}

	return "Unknown error";
}
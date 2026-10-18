/*
 * can_functions.c
 *
 *  Created on: 10 sep. 2018
 *      Author: Mark
 *
 *  UNIVERSAL: identical in Project_CAN_BL_G0B1 (bootloader) and the applications.
 *  Project specific settings (BOOTLOADER, SHELL) live in can_config.h
 */

#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "can_functions.h"
#include "fdcan.h"
#include "iwdg.h"
#include "ms_timers.h"
#include "uart_sci.h"


#if BOOTLOADER>0
#include "user_flash.h"
#include "crc.h"
#include "time.h"
#else
#include "vars.h"
#endif

/*
 ____    _    _   _   ____    ___    ____  ____  _____     _______ ____
 / ___|  / \  | \ | | |___ \  / _ \  |  _ \|  _ \|_ _\ \   / / ____|  _ \
 | |     / _ \ |  \| |   __) || | | | | | | | |_) || | \ \ / /|  _| | |_) |
 | |___ / ___ \| |\  |  / __/ | |_| | | |_| |  _ < | |  \ V / | |___|  _ <
 \____/_/   \_\_| \_| |_____(_)___/  |____/|_| \_\___|  \_/  |_____|_| \_\

 Interface:
 init:				FDCAN_Config
 send data:			can_send_msg, placed in the TX queue
 receive data:		HAL_FDCAN_RxFifo0Callback places it in the RX queue
 poll from main:	handle_can processes the RX queue and feeds the TX FIFO

 Settings (can_config.h):
 (X) Bootloader / Application
 (X) Shell

 */

#if BOOTLOADER>0
static uint8_t reboot_tmr = NO_TIMER;
#endif

// TX: producer = any context (push is irq safe), consumer = handle_can
static can_trx_t can_tx_queue[CAN_TX_QUEUE_SIZE];
static volatile uint32_t can_tx_in;
static volatile uint32_t can_tx_out;

// RX: producer = FDCAN interrupt, consumer = handle_can
static can_trx_t can_rx_queue[CAN_RX_QUEUE_SIZE];
static volatile uint32_t can_rx_in;
static volatile uint32_t can_rx_out;
static volatile uint32_t can_rx_overflow;	// frames lost because the RX queue was full

// Local functions
static bool compare_serial(const uint8_t *data);
static void can_100_procedure(const can_trx_t *msg);
static bool handle_can_tx(void);
static bool can_tx_push(const can_trx_t *msg);
static bool can_tx_pop(can_trx_t *msg);
static void can_tx_drop(void);
static bool can_rx_push(const can_trx_t *msg);
static bool can_rx_pop(can_trx_t *msg);

/**
 * @brief		CAN send message to Queue
 *
 * @param		id		11 bit ID
 * 					len		Data length 0..8 bytes
 * 					txdata	Pointer to data
 *
 * @retval	TRUE if placed in queue. When the queue is full and we run in thread
 * 			context, wait max 10 ms (pumping the queue to the FDCAN TX FIFO).
 * 			From an interrupt it never waits.
 */
bool can_send_msg(can_id_t id, uint8_t len, uint8_t *txdata)
{
	can_trx_t tx_data;

	if (len > 8)
		return false;

	if (__get_IPSR() == 0)
	{
		uint32_t start = HAL_GetTick();

		while ((can_tx_in - can_tx_out) >= CAN_TX_QUEUE_SIZE)
		{
			wdt_clr();
			handle_can_tx();
			if ((HAL_GetTick() - start) >= 10)
				break;
		}
	}

	tx_data.id = id & 0x7FF;
	tx_data.len = len;
	tx_data.rtr = FDCAN_DATA_FRAME;
	memset(tx_data.data, 0, sizeof(tx_data.data));
	if (txdata != NULL && len > 0)
		memcpy(tx_data.data, txdata, len);

	return can_tx_push(&tx_data);
}

/**
 * @brief  Rx FIFO 0 callback. The FIFO has only 3 elements, empty it completely
 *         and push to the RX queue, handle_can does the processing.
 * @param  hfdcan: pointer to an FDCAN_HandleTypeDef structure that contains
 *         the configuration information for the specified FDCAN.
 * @param  RxFifo0ITs: indicates which Rx FIFO 0 interrupts are signalled.
 *         This parameter can be any combination of @arg FDCAN_Rx_Fifo0_Interrupts.
 * @retval None
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
	FDCAN_RxHeaderTypeDef header;
	can_trx_t msg;

	if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0)
		return;

	while (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) > 0)
	{
		if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &header, msg.data) != HAL_OK)
			break;

		if (header.IdType != FDCAN_STANDARD_ID || header.RxFrameType != FDCAN_DATA_FRAME)
			continue;

		msg.id = header.Identifier;
		msg.len = header.DataLength <= 8 ? header.DataLength : 8;	// FDCAN_DLC_BYTES_x equals the byte count
		msg.rtr = FDCAN_DATA_FRAME;
		can_rx_push(&msg);
	}
}

/*
 * Hand one queued frame to the FDCAN TX FIFO
 */
static bool handle_can_tx(void)
{
	static can_trx_t msg;
	FDCAN_TxHeaderTypeDef header;

	if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) == 0)
		return false;

	if (!can_tx_pop(&msg))
		return false;

	header.Identifier = msg.id;
	header.IdType = FDCAN_STANDARD_ID;
	header.TxFrameType = msg.rtr;
	header.DataLength = msg.len;		// FDCAN_DLC_BYTES_x equals the byte count
	header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	header.BitRateSwitch = FDCAN_BRS_OFF;
	header.FDFormat = FDCAN_CLASSIC_CAN;
	header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	header.MessageMarker = 0;

	// only remove from the queue when accepted, otherwise retry on the next call
	if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &header, msg.data) != HAL_OK)
		return false;

	can_tx_drop();
	return true;
}

/**
 * @brief  CAN handler, poll from the main loop.
 *         Processes the received frames and feeds the TX FIFO.
 */
void handle_can(void)
{
	can_trx_t msg;

	// Bus-off recovery: the FDCAN sets INIT on bus-off and has no automatic recovery, clear it
	// to start the recovery sequence (128 x 11 recessive bits)
	if ((hfdcan1.Instance->PSR & FDCAN_PSR_BO) != 0U)
		CLEAR_BIT(hfdcan1.Instance->CCCR, FDCAN_CCCR_INIT);

	while (can_rx_pop(&msg))
	{
		// Goto Special Functions. Poll and ..
		if (msg.id < 100)
			can_100_procedure(&msg);

		/*
		 * Other RX actions: application specific (weak, see can_rx_message)
		 */
		can_rx_message(&msg);
	}

	while (handle_can_tx())
		;
}

/**
 * @brief  Application hook for every received data frame, called from handle_can (thread context).
 *         Weak: an application without own CAN messages does not need to implement it.
 */
__weak void can_rx_message(const can_trx_t *msg)
{
	(void) msg;
}

/**
 * @brief  Configures the FDCAN.
 * @param  None
 * @retval None
 */
void FDCAN_Config(void)
{
	/* No filters: accept all standard data frames in FIFO0,
	 reject extended and remote frames */
	if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_REJECT, FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE) != HAL_OK)
	{
		Error_Handler();
	}

	/* Start the FDCAN module */
	if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  basic CAN id handling for below 100 id
 *
 * @param  msg packet of received data
 *
 * @retval None
 */
static void can_100_procedure(const can_trx_t *msg)
{
	uint8_t buff[8] = { 0 };
	uint32_t *p_serial = (uint32_t*) RAM_SERIAL; // bootloader: set by ID_BL_SERIAL, application: fixed
#if BOOTLOADER>0
	static uint32_t blocks_cnt = 0;
	static bool communication = false;
#else
	volatile int *p_key = (int*) RAM_KEY;
	static bool shell_active = false; // Can be activated by CAN command

	// My serial on fixed RAM address, the bootloader reads it
	*p_serial = CAN_SERIAL;
#endif

	// response to POLL with 11
	if (msg->id == ID_POLL)
	{
		tty_puts("Poll\r\n");
		memcpy(buff, p_serial, 4);
		can_send_msg(ID_POLL_RPY, 4, buff);
	}

#if BOOTLOADER>0
	// Response to receive serial number
	if (msg->id == ID_BL_SERIAL)
	{
		memcpy(p_serial, msg->data, 4);
		tty_puts("Set serial\r\n");
		can_send_msg(ID_BL_RESULT, 4, (uint8_t*) p_serial);
	}

	// Response to JumpBoot
	if (msg->id == ID_BL_ACITVATE)
	{
		if (compare_serial(msg->data))
		{
			tty_puts("Already in bootloader\r\n");
			can_send_msg(ID_OC_RES, 4, (uint8_t*) p_serial);
		}
	}

	// Response to Open channel
	if (msg->id == ID_OPEN)
	{
		if (compare_serial(msg->data))
		{
			tty_puts("CAN Channel Open\r\n");
			communication = true;
			blocks_cnt = 0;
			can_send_msg(ID_OC_RES, 4, (uint8_t*) p_serial);
		}
	}

	// Response to Close communication
	if (msg->id == ID_CLOSE)
	{
		communication = false;

		if (compare_serial(msg->data))
		{
			tty_puts("CAN Close\r\n");
			can_send_msg(ID_OC_RES, 4, (uint8_t*) p_serial);
		}
	}

	// Response to Reset
	if (msg->id == ID_START)
	{
		if (compare_serial(msg->data) && communication)
		{
			stm32_flash_flush();
			tty_puts("Start App\r\n");
			buff[0] = 0;
			can_send_msg(ID_FL_RESULT, 4, buff);

			for (uint16_t var = 0; var < 200; ++var)
			{
				wdt_clr();
				handle_can();
				HAL_Delay(1);
			}
			// set reboot function
			JumptoApp();
		}
	}

	// Response to Flash erase
	if (msg->id == ID_ERASE)
	{
		if (compare_serial(msg->data) && communication)
		{
			tty_puts("CAN Flash Erase\r\n");
			blocks_cnt = 0;
			buff[0] = stm32_flash_erase(FLASH_APP_START_ADDR, APP_SIZE);
			can_send_msg(ID_FL_RESULT, 1, buff);
		}
	}

	// Receive data
	if (msg->id == ID_FL_DATA)
	{
		if (communication)
		{
			buff[0] = stm32_flash_write(FLASH_APP_START_ADDR + blocks_cnt * 8, (uint8_t*) msg->data, 8);
			buff[0] ^= 1;
			blocks_cnt++;

			can_send_msg(ID_FL_RESULT, 1, buff);
		}
	}

	if (msg->id == ID_ASK_CRC)
	{
		app_info_t app_info;
		app_info_t *p_app_info = (app_info_t*) FLASH_APP_START_ADDR;

		if (compare_serial(msg->data) && communication)
		{
			stm32_flash_flush();	// write last partial quad-word
			tty_puts("CAN check CRC\r\n");
			crc_verify_flash(&app_info, FLASH_APP_START_ADDR, APP_SIZE);

			buff[0] = (app_info.crc32 >> 24) & 0xFF;
			buff[1] = (app_info.crc32 >> 16) & 0xFF;
			buff[2] = (app_info.crc32 >> 8) & 0xFF;
			buff[3] = (app_info.crc32 >> 0) & 0xFF;
			buff[4] = (p_app_info->crc32 >> 24) & 0xFF;
			buff[5] = (p_app_info->crc32 >> 16) & 0xFF;
			buff[6] = (p_app_info->crc32 >> 8) & 0xFF;
			buff[7] = (p_app_info->crc32 >> 0) & 0xFF;

			can_send_msg(ID_CRC, 8, buff);
		}
	}
#else
	// APPLICATION:
	// Response to JumpBoot
	if (msg->id == ID_BL_ACITVATE)
	{
		if (compare_serial(msg->data))
		{
			*p_key = CAN_KEY;
			*p_serial = CAN_SERIAL;

			NVIC_SystemReset();
		}
	}
#endif
#if SHELL>0

	// Response to Open Shell
	if (msg->id == ID_SH_OPEN)
	{
		if (compare_serial(msg->data))
		{
			shell_active = true;

			tty_puts("CAN SH Open\r\n");
			memcpy(buff, p_serial, 4);
			can_send_msg(ID_SH_RES, 4, buff);
			shell_use_can();
		}
	}

	// Response to Close Shell
	if (msg->id == ID_SH_CLOSE)
	{
		shell_active = false;

		if (compare_serial(msg->data))
		{
			shell_use_sci1();
			tty_puts("CAN SH Close\r\n");

			memcpy(buff, p_serial, 4);
			can_send_msg(ID_SH_RES, 4, buff);
		}
	}

	// Response to RX data
	if (msg->id == ID_SH_RX)
	{
		if (shell_active)
		{
			for (uint8_t var = 0; var < msg->len; ++var)
				sci_can_keyboard(msg->data[var]);
		}
	}
#endif
}

/**
 * @brief  Compare own CAN id (RAM) with argument
 *
 * @param  4 byte ID
 *
 * @retval TRUE if identical
 */
static bool compare_serial(const uint8_t *data)
{
	uint32_t *p_serial = (uint32_t*) RAM_SERIAL;
	return (*p_serial == ((uint32_t) data[3] << 24 | (uint32_t) data[2] << 16 | (uint32_t) data[1] << 8 | (uint32_t) data[0]));
}

/*
 * TX queue. Push may be called from main and from interrupts
 */
static bool can_tx_push(const can_trx_t *msg)
{
	bool ok = false;
	uint32_t primask = __get_PRIMASK();

	__disable_irq();
	if ((can_tx_in - can_tx_out) < CAN_TX_QUEUE_SIZE)
	{
		can_tx_queue[can_tx_in & CAN_TX_QUEUE_MASK] = *msg;
		can_tx_in++;
		ok = true;
	}
	__set_PRIMASK(primask);

	return ok;
}

static bool can_tx_pop(can_trx_t *msg)
{
	if (can_tx_in == can_tx_out)
		return false;

	*msg = can_tx_queue[can_tx_out & CAN_TX_QUEUE_MASK];
	return true;
}

static void can_tx_drop(void)
{
	can_tx_out++;
}

/*
 * RX queue. Single producer (interrupt), single consumer (main)
 */
static bool can_rx_push(const can_trx_t *msg)
{
	if ((can_rx_in - can_rx_out) >= CAN_RX_QUEUE_SIZE)
	{
		can_rx_overflow++;
		return false;
	}

	can_rx_queue[can_rx_in & CAN_RX_QUEUE_MASK] = *msg;
	can_rx_in++;
	return true;
}

static bool can_rx_pop(can_trx_t *msg)
{
	if (can_rx_in == can_rx_out)
		return false;

	*msg = can_rx_queue[can_rx_out & CAN_RX_QUEUE_MASK];
	can_rx_out++;
	return true;
}

/*
 * can_functions.h
 *
 *  Created on: 10 sep. 2018
 *      Author: Mark
 *
 *  UNIVERSAL: identical in Project_CAN_BL_G0B1 (bootloader) and the applications.
 *  Project specific settings (BOOTLOADER, SHELL) live in can_config.h
 */

#ifndef CAN100_H_
#define CAN100_H_

#include <stdint.h>
#include <stdbool.h>


#include "../../../Project_Gateway/inc/can_id.h"

#define BOOTLOADER			(1) // 1 = This is the bootloader, 0 = application
#define SHELL				(0) // Include shell CAN commands

#define INRQ 1
#define INAK 1

#define CAN_TX_QUEUE_SIZE	16		// must be a power of 2
#define CAN_TX_QUEUE_MASK	(CAN_TX_QUEUE_SIZE - 1)
#define CAN_RX_QUEUE_SIZE	32		// must be a power of 2
#define CAN_RX_QUEUE_MASK	(CAN_RX_QUEUE_SIZE - 1)

typedef struct
{
	can_id_t id;
	uint8_t len;
	uint8_t rtr;
	uint8_t data[8];
} can_trx_t;

/*
 * Global functions
 */

bool can_send_msg(can_id_t id, uint8_t len, uint8_t * txdata);
void FDCAN_Config(void);
void handle_can(void);
void can_rx_message(const can_trx_t *msg);	// weak, implement in the application for own messages

#endif /* CAN100_H_ */

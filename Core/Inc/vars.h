/*
 * vars.h
 *
 *  Created on: 6 mrt 2026
 *      Author: mark.ursum
 */

#ifndef INC_VARS_H_
#define INC_VARS_H_

// Application info
typedef struct s_app_info
{
  uint32_t key;
  uint32_t version;
  uint32_t crc32;
  uint32_t size;
  uint8_t build_date[sizeof(__DATE__)];
  uint8_t build_time[sizeof(__TIME__)];
  uint8_t dummy[3];
} app_info_t;

#define APP_KEY   (0xAA55AA55)

#define VERSION_KEY_JTC	0xC2

#define VERSION   (0x0001FF00 | VERSION_KEY_JTC)

#define WAIT_KEY  	(0x55AA55AA)
#define WAIT_KEY2	(0x55AA55A2)

#define CRASH_KEY 	(0x55AA55CC)
#define CAN_KEY		(0x112255AA)	// RAM_KEY value: jump to the CAN bootloader

#define RAM_KEY   	(0x20000000) // RAM address
#define RAM_SERIAL	(0x20000004)

#endif /* INC_VARS_H_ */

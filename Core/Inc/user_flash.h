/*
 * flash.h
 *
 *  Created on: 24 jan. 2017
 *      Author: Mark Ursum
 */

#ifndef FLASH_H_
#define FLASH_H_
#include "main.h"
#include "stdbool.h"


HAL_StatusTypeDef stm32_flash_erase(uint32_t start, uint32_t size);
bool stm32_ymodem_flash_write(void *p_args);
bool stm32_flash_write(uint32_t address, const uint8_t *p_data, uint32_t size);
bool stm32_flash_flush(void);


// STM32U575VIT has 2Mb of flash (0x08000000 - 0x081FFFFF), dual bank:
// bank 1 = 0x08000000 - 0x080FFFFF (pages 0-127), bank 2 = 0x08100000 - 0x081FFFFF (pages 0-127)
// pages are 8Kb, programming is per quad-word (16 bytes).
// bootloader					=	64Kb   (pages 0-7)
// Application					=	1984Kb (pages 8-255), fills the rest of the device.
//   app_info header at the start, vector table at _main_app_start_address (linker script)

#define FLASH_QUADWORD_SIZE 16U

#define FLASH_TOTAL_SIZE			0x00200000U	// 2Mb
#define FLASH_PAGES_TOTAL			(FLASH_TOTAL_SIZE / 0x2000U)	// 256 pages of 8Kb

/* Base address of absolute flash page n (0..255, bank 2 starts at page 128) */
#define ADDR_FLASH_PAGE(n)			((uint32_t)(0x08000000U + (uint32_t)(n) * 0x2000U))

#define ADDR_FLASH_PAGE_8			ADDR_FLASH_PAGE(8)		// 0x08010000, end of bootloader
#define ADDR_FLASH_PAGE_128			ADDR_FLASH_PAGE(128)	// 0x08100000, bank 2
#define ADDR_FLASH_PAGE_256			ADDR_FLASH_PAGE(256)	// 0x08200000, end of flash

#define FLASH_APP_START_ADDR		ADDR_FLASH_PAGE_8		// at 64kb
#define FLASH_APP_END_ADDR			ADDR_FLASH_PAGE_128 //256		// end of flash

#define APP_SIZE					(FLASH_APP_END_ADDR - FLASH_APP_START_ADDR) // 1984Kb



/* Exported functions ------------------------------------------------------- */

#endif /* FLASH_H_ */

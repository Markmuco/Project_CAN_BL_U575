/*
 * flash.c
 *
 *  Created on: 24 jan. 2026
 *      Author: Mark Ursum
 *
 *  STM32U575 (2Mb, dual bank, 8Kb pages, quad-word (128 bit) programming)
 */

#include "main.h"
#include "user_flash.h"
#include "string.h"
#include "ymodem.h"
#include "stdbool.h"
#include "iwdg.h"
#include "n25lpxx.h"

/* Staging buffer: STM32U5 flash is programmed per quad-word (128 bit) and a quad-word can only
 * be programmed once. Writes smaller than 16 bytes (CAN: 8 bytes) are collected here. */
static uint32_t qw_base = 0xFFFFFFFFU;		// address of the pending quad-word
static uint8_t qw_buf[FLASH_QUADWORD_SIZE] __attribute__((aligned(4)));
static bool qw_dirty = false;

static bool program_quadword(uint32_t address, const uint8_t *p_qw);

/*!
 * \brief Erase flash page by page (8Kb pages, both banks).
 *
 * \param start   Start address, inside a page.
 * \param size    Number of bytes to erase.
 *
 * \return HAL_StatusTypeDef
 */
HAL_StatusTypeDef stm32_flash_erase(uint32_t start, uint32_t size)
{
	FLASH_EraseInitTypeDef erase;
	uint32_t page_error = 0;
	HAL_StatusTypeDef st = HAL_OK;

	if (size == 0)
		return HAL_OK;

	qw_dirty = false;
	qw_base = 0xFFFFFFFFU;

	uint32_t first = (start - FLASH_BASE) / FLASH_PAGE_SIZE;				// absolute page 0..255
	uint32_t last = (start + size - 1U - FLASH_BASE) / FLASH_PAGE_SIZE;
	uint32_t pages_per_bank = FLASH_BANK_SIZE / FLASH_PAGE_SIZE;

	/* Disable instruction cache prior to internal cacheable memory update */
	if (HAL_ICACHE_Disable() != HAL_OK)
		Error_Handler();

	HAL_FLASH_Unlock();

	for (uint32_t pg = first; pg <= last; pg++)
	{
		wdt_clr();

		erase.TypeErase = FLASH_TYPEERASE_PAGES;
		erase.Banks = (pg < pages_per_bank) ? FLASH_BANK_1 : FLASH_BANK_2;
		erase.Page = pg % pages_per_bank;
		erase.NbPages = 1;

		st = HAL_FLASHEx_Erase(&erase, &page_error);
		if (st != HAL_OK)
			break;
	}

	HAL_FLASH_Lock();

	/* Re-enable instruction cache */
	if (HAL_ICACHE_Enable() != HAL_OK)
		Error_Handler();

	return st;
}

/*!
 * \brief Program one complete quad-word (16 bytes) at a 16 byte aligned address.
 */
static bool program_quadword(uint32_t address, const uint8_t *p_qw)
{
	HAL_StatusTypeDef st;

	if (HAL_ICACHE_Disable() != HAL_OK)
		Error_Handler();

	if (HAL_FLASH_Unlock() != HAL_OK)
	{
		HAL_ICACHE_Enable();
		return false;
	}

	st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, address, (uint32_t) p_qw);

	HAL_FLASH_Lock();

	if (HAL_ICACHE_Enable() != HAL_OK)
		Error_Handler();

	return (st == HAL_OK);
}

/*!
 * \brief Write the pending (partly filled) quad-word, padded with 0xFF.
 *
 * \return - true on successful
 */
bool stm32_flash_flush(void)
{
	bool ok = true;

	if (qw_dirty)
	{
		ok = program_quadword(qw_base, qw_buf);
		qw_dirty = false;
		qw_base = 0xFFFFFFFFU;
	}
	return ok;
}

/*!
 * \brief Write data to the application memory.
 *
 * \param address   Start address of write (multiple of 8, or contiguous with the previous write).
 * \param p_data   Pointer to data to write.
 * \param size   Size of data to write.
 *
 * \return - true on successful
 *
 * \note The flash memory must be erased before it can be written.
 * \note Data that does not fill a complete quad-word stays pending until the next
 *       contiguous write or stm32_flash_flush().
 */
bool stm32_flash_write(uint32_t address, const uint8_t *p_data, uint32_t size)
{
	if ((address & 0x7) != 0)
		return false;

	while (size > 0)
	{
		uint32_t base = address & ~(FLASH_QUADWORD_SIZE - 1U);
		uint32_t offs = address - base;

		/* Pending quad-word belongs to another address: write it first */
		if (qw_dirty && qw_base != base)
		{
			if (!stm32_flash_flush())
				return false;
		}
		if (!qw_dirty)
		{
			memset(qw_buf, 0xFF, sizeof(qw_buf));
			qw_base = base;
		}

		uint32_t chunk = FLASH_QUADWORD_SIZE - offs;
		if (chunk > size)
			chunk = size;
		memcpy(&qw_buf[offs], p_data, chunk);
		qw_dirty = true;

		/* Quad-word complete */
		if (offs + chunk == FLASH_QUADWORD_SIZE)
		{
			if (!stm32_flash_flush())
				return false;
		}

		address += chunk;
		p_data += chunk;
		size -= chunk;
	}

	return true;
}

/*!
 * \brief Wrapper function for YMODEM.
 *
 * \param p_args   Pointer to arguments structure.
 *
 * \return - true on successful
 *
 * \note The flash memory must be erased before it can be written.
 */
bool stm32_ymodem_flash_write(void *p_args)
{
	ym_data_t ym_data;

	// Cast p_args back to a ym_data_t struct
	ym_data = *((ym_data_t*) p_args);

	// Ymodem blocks are separate transfers: do not keep a partial quad-word pending
	return (stm32_flash_write(ym_data.addr, ym_data.p_buf, ym_data.len) && stm32_flash_flush());
}

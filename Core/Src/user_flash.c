/*
 * flash.c
 *
 *  Created on: 24 jan. 2026
 *      Author: Mark Ursum
 *
 */

#include "main.h"
#include "user_flash.h"
#include "string.h"
#include "ymodem.h"
#include "stdbool.h"
#include "iwdg.h"
#include "n25lpxx.h"

static FLASH_EraseInitTypeDef EraseInitStruct;

static uint32_t GetSector(uint32_t Addr);


/*!
 * \brief Erase the application memory.
 *
 * \param -
 *
 * \return HAL_StatusTypeDef
 */
HAL_StatusTypeDef stm32_flash_erase(uint32_t start, uint32_t size)
{
	uint32_t FirstSector = 0, NbOfSectors = 0;
	uint32_t SectorError = 0;
	uint32_t BankNumber = 0;

	/* Get the 1st sector to erase */
	FirstSector = GetSector(start);

	/* Get the number of sectors to erase from 1st sector */
	NbOfSectors = GetSector(start + size - 1U) - FirstSector + 1U;

	/* Get the bank */
	BankNumber = FLASH_BANK_1; // FLASH_BANK_BOTH

	/* Fill EraseInit structure*/
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.Banks = BankNumber;
	EraseInitStruct.Sector = FirstSector;
	EraseInitStruct.NbSectors = NbOfSectors;

	/* Disable instruction cache prior to internal cacheable memory update */
	if (HAL_ICACHE_Disable() != HAL_OK)
		Error_Handler();

	HAL_FLASH_Unlock();

	FLASH_WaitForLastOperation(0xFFFF);

	/* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
	 you have to make sure that these data are rewritten before they are accessed during code
	 execution. If this cannot be done safely, it is recommended to flush the caches by setting the
	 DCRST and ICRST bits in the FLASH_CR register. */
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
	{
		/*
		 Error occurred while sector erase.
		 User can add here some code to deal with this error.
		 SectorError will contain the faulty sector and then to know the code error on this sector,
		 user can call function 'HAL_FLASH_GetError()'
		 */
		return (HAL_ERROR);
	}

	HAL_FLASH_Lock();

	/* Re-enable instruction cache */
	if (HAL_ICACHE_Enable() != HAL_OK)
		Error_Handler();

	return HAL_OK;
}

/*!
 * \brief Write data to the application memory.
 *
 * \param address   Start address of write.
 * \param p_data   Pointer to data to write.
 * \param size   Size of data to write.
 *
 * \return - true on successful
 *
 * \note The flash memory must be erased before it can be written.
 */
bool stm32_flash_write(uint32_t address, const uint8_t *p_data, uint32_t size)
{
	HAL_StatusTypeDef st = HAL_OK;
	uint8_t temp[16] =
	{ 0 };

	if ((address & 0xF) != 0)
		return false;
	if (size == 0)
		return true;

	if (HAL_ICACHE_Disable() != HAL_OK)
		Error_Handler();
	if (HAL_FLASH_Unlock() != HAL_OK)
		return false;

	FLASH_WaitForLastOperation(0xFFFF);

	while (size > 0)
	{
		uint32_t chunk = (size >= 16U) ? 16U : size;
		memset(temp, 0xFF, sizeof(temp));   // of 0x00, afhankelijk van protocol
		memcpy(temp, p_data, chunk);

		st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, address, (uint32_t) temp);
		if (st != HAL_OK)
			break;

		address += 16U;
		p_data += chunk;
		size -= chunk;
	}

	HAL_FLASH_Lock();

	if (HAL_ICACHE_Enable() != HAL_OK)
		Error_Handler();

	return (st == HAL_OK);
}

/**
 * @brief  Gets the sector of a given address
 * @param  Addr: Address of the FLASH Memory
 * @retval The sector of a given address
 */
static uint32_t GetSector(uint32_t Addr)
{
	uint32_t sector = 0;

	if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
	{
		/* Bank 1 */
		sector = (Addr - FLASH_BASE) / FLASH_SECTOR_SIZE;
	}
	else
	{
		/* Bank 2 */
		sector = (Addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_SECTOR_SIZE;
	}

	return sector;
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

	// Perform flash write
	return (stm32_flash_write(ym_data.addr, ym_data.p_buf, ym_data.len));
}


/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    crc.c
  * @brief   This file provides code for the configuration
  *          of the CRC instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "crc.h"

/* USER CODE BEGIN 0 */

#include "user_flash.h"
#include "string.h"

/* USER CODE END 0 */

CRC_HandleTypeDef hcrc;

/* CRC init function */
void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_WORDS;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

void HAL_CRC_MspInit(CRC_HandleTypeDef* crcHandle)
{

  if(crcHandle->Instance==CRC)
  {
  /* USER CODE BEGIN CRC_MspInit 0 */

  /* USER CODE END CRC_MspInit 0 */
    /* CRC clock enable */
    __HAL_RCC_CRC_CLK_ENABLE();
  /* USER CODE BEGIN CRC_MspInit 1 */

  /* USER CODE END CRC_MspInit 1 */
  }
}

void HAL_CRC_MspDeInit(CRC_HandleTypeDef* crcHandle)
{

  if(crcHandle->Instance==CRC)
  {
  /* USER CODE BEGIN CRC_MspDeInit 0 */

  /* USER CODE END CRC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CRC_CLK_DISABLE();
  /* USER CODE BEGIN CRC_MspDeInit 1 */

  /* USER CODE END CRC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */


/*!
 * \brief This function checks the integrity of a segment in flash memory.
 *
 * \param address The start address of the segment.
 * \param size The maximum size of the segment.
 *
 * \return	0 OK
 * 					1 CRC error
 * 					2 No file
 */
uint8_t crc_verify_flash(app_info_t  * app_info, uint32_t address, uint32_t size)
{
	app_info_t *p_app_info = (app_info_t*) address;
	uint32_t crc32 = 0;

	if ((p_app_info->key == APP_KEY) && (p_app_info->size < APP_SIZE))
	{
		__HAL_CRC_DR_RESET(&hcrc);

		// Verify integrity.
		memcpy(app_info, (uint8_t *) (address), sizeof(app_info_t));
		app_info->crc32 = 0xFFFFFFFF;
		app_info->size = 0xFFFFFFFF;
		HAL_CRC_Accumulate(&hcrc, (uint32_t *) app_info, (sizeof(app_info_t) / 4));

		app_info->crc32 = p_app_info->crc32;
		app_info->size = p_app_info->size;
		crc32 = HAL_CRC_Accumulate(&hcrc, (uint32_t *) (address + sizeof(app_info_t)), (p_app_info->size - sizeof(app_info_t)) / 4);

	//	sci1_printf("NEW 0x%08X 0x%08X\r\n", crc32, p_app_info->crc32);

		if (crc32 == p_app_info->crc32)
			return 0;
		else
			return 1;
	}
	else
		return 2;
}


/* USER CODE END 1 */


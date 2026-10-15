/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 ** This notice applies to any and all portions of this file
 * that are not between comment pairs USER CODE BEGIN and
 * USER CODE END. Other portions of this file, whether
 * inserted by the user or by software development tools
 * are owned by their respective copyright owners.
 *
 * COPYRIGHT(c) 2019 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "fdcan.h"
#include "icache.h"
#include "iwdg.h"
#include "rtc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ms_timers.h"
#include "shell.h"

#include "crc.h"
#include "uart_sci.h"
#include "utilities.h"
#include "user_flash.h"
#include "can_functions.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/*

 DESCRIPTION:

 REMARKS:

 arm-none-eabi-objcopy -O binary "${BuildArtifactFileBaseName}.elf" "${BuildArtifactFileBaseName}.bin" && arm-none-eabi-size "${BuildArtifactFileName}";${ProjDirPath}/writeCRC.exe "${BuildArtifactFileBaseName}.bin"

 - section MYSEC
 - .data:  *(.RamFunc)
 - Optimize more (-O2)

 NOTES
 CPU freq 			200Mhz
 Uart1 COM port		115K
 Uart2 RS485		921600
 SPI1 Flash 2Mb 	12.5MBit
 IWDT 40khz/4095/16 4.096 sec

 TOOLS
 - CubeMX 6.17.0
 - CubeIDE 2.2.0

 PRINCIPE
 0x08000000 Bootloader 32K
 0x08008000 Application 55kb
 0x08010000 Shadow flash 55kb

 VERSION OVERVIEW:
 0.01
 - eerste versie


 */
uint8_t toggle_tmr = NO_TIMER;
uint8_t autostart_tmr = NO_TIMER;


/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*!< Info structure for bootloader */
volatile const app_info_t __attribute__((section (".mySection"))) c_app_info =
{ .key = APP_KEY,				// Key
		.version = VERSION,		// Version
		.crc32 = 0xFFFFFFFF,		// CCITT-CRC32
		.size = 0xFFFFFFFF,		// File size
		.build_date = __DATE__,	// Build date
		.build_time = __TIME__,	// Build time
		.dummy =
		{ 0xFF, 0xFF, 0xFF } // Dummy[3]
};

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

	//__disable_irq();
	app_info_t *p_app_info = (app_info_t*) FLASH_APP_START_ADDR;
//	app_info_t *p_shadow_info = (app_info_t*) FLASH_SHADOW_START_ADDR;
	uint32_t *ram_key = (uint32_t*) RAM_KEY;

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CRC_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();
  MX_ICACHE_Init();
  MX_FDCAN1_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */

	__enable_irq();

	// Open the byteQ
	init_sci();

	// setup the function timer
	timer_open();

	// init the Shell
	shell_open();

	FDCAN_Config();

	if (*ram_key == WAIT_KEY)
	{
		tty_puts("'JOZ' (@) 2026 STM32U JT_BL, Stop\r\n");
		autostart_tmr = timer_get();
		timer_start(autostart_tmr, 30000, NULL);
	}
	else if (*ram_key == CRASH_KEY)
	{
		tty_puts("'JOZ' (@) 2026 STM32U0 JT_BL, Crash\r\n");
		autostart_tmr = timer_get();
		timer_start(autostart_tmr, 2500, NULL);
	}
	else if (p_app_info->key == APP_KEY)
	{
		tty_puts("'JOZ' (@) 2026 STM32U0 JT_BL, Start\r\n");
		autostart_tmr = timer_get();
		timer_start(autostart_tmr, 1500, NULL);
	}
	else
		tty_puts("'JOZ' (@) 2026 STM32U0 JT_BL\r\n");

	// APP need to clear this flag or else app was crashed
	*ram_key = CRASH_KEY;

	app_info_t app_info;
	app_info_t shadow_info;
	app_info_t shadow_hdr;
#if 0
	if ((n25lpxx_read((uint8_t*) &shadow_hdr, N25LPXX_SHADOW_START_ADDR, sizeof(shadow_hdr)) == FLS_SUCCESS) && (shadow_hdr.key == APP_KEY))
	{
		// Verify integrity of shadow
		if (crc_verify_n25lpxx(&shadow_info, N25LPXX_SHADOW_START_ADDR, N25LPXX_SHADOW_SIZE) == 0)
		{
			// get CRC of application
			crc_verify_flash(&app_info, FLASH_APP_START_ADDR, APP_SIZE);
			if ((shadow_info.version & 0xFF) == VERSION_KEY_JTC)
			{
				if (app_info.crc32 != shadow_info.crc32)
				{
					tty_puts("Copy Shadow to App..");
					//NVIC_DisableIRQ(USART3_4_LPUART1_IRQn); // IRQs from the main will stall copying
					HAL_Delay(10);

					if (!copy_shadow_n25lpxx(APP_SIZE))
					{
						timer_free(&autostart_tmr);
						tty_puts("Error\r\n");
					}
					else
					{
						// Delete the header
						tty_puts("Erasing shadow flash... ");
						HAL_Delay(10);
						if (n25lpxx_erase(N25LPXX_SHADOW_START_ADDR, N25LPXX_SHADOW_SIZE) == FLS_SUCCESS)
						{
							sci1_puts("OK\r\n");
							autostart_tmr = timer_get();
							timer_start(autostart_tmr, 250, NULL);
						}
						else
						sci1_puts("Error\r\n");

					}
				}
			}
			else
				tty_puts("No MCM C9 code\r\n");
		}
		else
			tty_puts("Shadow CRC error\r\n");
	}
#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

//	app_info_t app_info;

	while (1)
	{
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		wdt_clr();

		shell_process();

		handle_can();

		if (timer_elapsed(autostart_tmr))
		{
			timer_free(&autostart_tmr);

			// Verify integrity.
			if (crc_verify_flash(&app_info, FLASH_APP_START_ADDR, APP_SIZE) == 0)
			{
				tty_puts("Jump\r\n");
				HAL_Delay(15);
				JumptoApp();
			}
			else
				tty_puts("APP CRC error\r\n");

		}
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.LSIDiv = RCC_LSI_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 20;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 1;
  RCC_OscInitStruct.PLL.PLLR = 1;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  file: The file name as string.
 * @param  line: The line in file as a number.
 * @retval None
 */
void _Error_Handler(char *file, int line)
{
	char buf[10];

	tty_puts("FATAL error File:");
	tty_puts(file);
	tty_puts(" Line ");
	tty_puts(my_itoa(line, buf, 10));
	tty_puts("\r\n");

	while (1)
	{
	}
}



typedef void (*pFunction)(void);

/* Defined in the linker script */
extern uint32_t _main_app_start_address;

/**
 * @brief  This function is responsible for starting the main application.
 * @retval None
 */
void JumptoApp(void)
{
    uint32_t appAddress = (uint32_t)&_main_app_start_address;
    uint32_t appStack   = *(__IO uint32_t *)appAddress;
    uint32_t appEntry   = *(__IO uint32_t *)(appAddress + 4U);
    pFunction appResetHandler = (pFunction)appEntry;

    if ((appStack & 0x2FF00000U) != 0x20000000U)
        return;

    if (((appEntry & 0xFF000000U) != 0x08000000U) || ((appEntry & 1U) == 0U))
        return;

    __disable_irq();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    for (uint32_t i = 0; i < 8U; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFFU;
        NVIC->ICPR[i] = 0xFFFFFFFFU;
    }

    HAL_DeInit();
    HAL_RCC_DeInit();
    /* Important for STM32H5 */
    HAL_ICACHE_DeInit();

    __set_CONTROL(0U);
    __set_PSP(0U);
    __set_BASEPRI(0U);
    __set_FAULTMASK(0U);

    SCB->VTOR = appAddress;

    __DSB();
    __ISB();

    __set_MSP(appStack);

    __DSB();
    __ISB();

    appResetHandler();

    while (1)
    {
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	while (1)
	{
	}
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

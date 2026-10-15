/*
 * shell.c
 *
 *  Created on: 16 mrt. 2018
 *      Author: Mark
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "main.h"
#include "usart.h"
#include "main.h"
#include "shell.h"
#include "crc.h"
#include "ms_timers.h"
#include "ymodem.h"
#include "utilities.h"
#include "uart_sci.h"
#include "iwdg.h"
#include "user_flash.h"

// connection to globals

// Private variables.
static uint8_t tmr = NO_TIMER;
static char mem_pool[SHELL_CMDLINE_SIZE * (SHELL_CMDLINE_HIST_COUNT + 1)];
static cmdline_obj_t cmdline;

// Private function prototypes.
static void sh_help(char *param);
static void sh_reboot(char *param);
void sh_ver(char *param);
void sh_crc(char *param);
static void sh_ea(char *param);
static void sh_ua(char *param);

static void sh_vi(char *param);
static void sh_st(char *param);


// Internal Functions
static bool shell_strcmp(const char *s1, const char *s2);
static void shell_cmdline_init(cmdline_obj_t *object);
static void shell_cmdline_add(cmdline_obj_t *object);
#if (SHELL_CMDLINE_HIST_COUNT > 0)
static void shell_cmdline_get(cmdline_obj_t *object);
static void shell_cmdline_get_prev(cmdline_obj_t *object);
static void shell_cmdline_get_next(cmdline_obj_t *object);
#endif
static bool shell_cmdline_execute(cmdline_obj_t *object);
static bool shell_tabline_find(cmdline_obj_t *object);
static void shell_tabline_get(cmdline_obj_t *object);

// Private constants.
static const cmd_tbl_t cmd_tbl[] =
{
{ "help", "This tekst", sh_help },
{ "ver", "Software version", sh_ver },
{ "reboot", "reboot CPU", sh_reboot },

{ "crc", "CRC32 check", sh_crc },

{ "ea", "Erase application", sh_ea },
{ "ua", "Upload app (Y-modem)", sh_ua },

{ "st", "Start application", sh_st },
{ "vi", "Version information", sh_vi },


};

#define CMD_TBL_COUNT   (sizeof(cmd_tbl) / sizeof(cmd_tbl[0]))

/*!
 * \brief This function opens the shell.
 *
 * \param -.
 *
 * \return -.
 */
void shell_open(void)
{
	if (tmr == NO_TIMER)
		tmr = timer_get();

	shell_cmdline_init(&cmdline);
}

/*!
 * \brief This function closes the shell.
 *
 * \param -.
 *
 * \return -.
 */
void shell_close(void)
{
	timer_free(&tmr);
}

/*!
 * \brief This function handles the shell.
 *
 * \param -.
 *
 * \return -.
 */
void shell_process(void)
{
	uint8_t c;

	if (shell_getc(&c))
	{
		switch (c)
		{
#if (SHELL_CMDLINE_HIST_COUNT > 0)
		case VK_UPARROW:
		// Get the previous item from the history list.
		shell_cmdline_get_prev(&cmdline);

		// Handle cursor position.
		tty_puts("\x1B[2K");// VT100 erase line.
		tty_putc('\r');
		tty_putsn((uint8_t *) cmdline.p_cmdline->str, cmdline.p_cmdline->size);
		break;

		case VK_DOWNARROW:
		// Get the next item from the history list.
		shell_cmdline_get_next(&cmdline);

		// Handle cursor position.
		tty_puts("\x1B[2K");// VT100 erase line.
		tty_putc('\r');
		tty_putsn((uint8_t *) cmdline.p_cmdline->str, cmdline.p_cmdline->size);
		break;
#endif
		case VK_BS:
			if (cmdline.p_cmdline->size)
			{
				// Copy tab line to command line.
				shell_tabline_get(&cmdline);

				// Handle cursor position.
				tty_puts("\x1B[D"); // VT100 cursor backward.
				tty_puts("\x1B[K"); // VT100 erase end of line.

				cmdline.p_cmdline->size--;
			}
			break;

		case VK_HT:
			if (cmdline.p_cmdline->size)
			{
				if (shell_tabline_find(&cmdline))
				{
					// Handle cursor position.
					tty_puts("\x1B[2K"); // VT100 erase line.
					tty_putc('\r');
					tty_putsn((uint8_t *) cmdline.tabline.str, cmdline.tabline.size);
				}
			}
			break;

		case VK_CR:
			// Handle cursor position.
			tty_puts("\r\n");

			if (cmdline.p_cmdline->size)
			{
				// Copy tab line to command line.
				shell_tabline_get(&cmdline);
				cmdline.p_cmdline->str[cmdline.p_cmdline->size] = '\0';

				if (!shell_cmdline_execute(&cmdline))
					tty_puts("not recognized as an internal or external command.\r\n");

				shell_cmdline_add(&cmdline);
			}
			break;

		case VK_ESC:
			if (cmdline.p_cmdline->size)
			{
				// Erase tab line.
				shell_tabline_get(&cmdline);

				// Handle cursor position.
				tty_puts("\x1B[2K"); // VT100 erase line.
				tty_putc('\r');

				cmdline.p_cmdline->size = 0;
			}
			break;

		default:
			if ((cmdline.p_cmdline->size < (SHELL_CMDLINE_SIZE - 1)) && (c >= 32) && (c < 127))
			{
				// Copy tab line to command line.
				shell_tabline_get(&cmdline);

				// Handle cursor position.
				tty_putc(c);

				cmdline.p_cmdline->str[cmdline.p_cmdline->size++] = c;
			}
			break;
		}
	}
}

/*!
 * \brief This function reads and filters characters from the TTY port.
 *
 * \param c Pointer to the character read.
 *
 * \return True if successful, false on error.
 *
 * \note The following virtual keys are supported:
 *
 *   1B 4F 50       : NUMLOCK
 *   1B 4F 51       : SLASH
 *   1B 4F 52       : MULTIPLY
 *   1B 4F 53       : MINUS
 *   1B 5B 31 7E    : HOME
 *   1B 5B 31 31 7E : F1
 *   1B 5B 31 32 7E : F2
 *   1B 5B 31 33 7E : F3
 *   1B 5B 31 34 7E : F4
 *   1B 5B 31 35 7E : F5
 *   1B 5B 31 37 7E : F6
 *   1B 5B 31 38 7E : F7
 *   1B 5B 31 39 7E : F8
 *   1B 5B 32 7E    : INS
 *   1B 5B 32 30 7E : F9
 *   1B 5B 32 31 7E : F10
 *   1B 5B 32 33 7E : F11
 *   1B 5B 32 34 7E : F12
 *   1B 5B 34 7E    : END
 *   1B 5B 35 7E    : PGUP
 *   1B 5B 36 7E    : PGDN
 *   1B 5B 41       : UPARROW
 *   1B 5B 42       : DOWNARROW
 *   1B 5B 43       : RIGHTARROW
 *   1B 5B 44       : LEFTARROW
 */
bool shell_getc(uint8_t *c)
{
	static uint8_t state = 0;
	static uint8_t c_tmp;
	uint8_t c_raw;
	bool res = false;

	if (tty_getc(&c_raw))
	{
		switch (state)
		{
		case 0:
			switch (c_raw)
			{
			case 0x1B:
				timer_start(tmr, ESC_DELAY, NULL);
				state = 1;
				break;

			default:
				*c = c_raw;
				res = true;
				break;
			}
			break;

		case 1:
			switch (c_raw)
			{
			case 0x4F:
				state = 2;
				break;

			case 0x5B:
				state = 3;
				break;

			default:
				state = 0;
				break;
			}
			break;

		case 2:
			switch (c_raw)
			{
			case 0x50:
				*c = VK_NUMLOCK;
				res = true;
				state = 0;
				break;

			case 0x51:
				*c = VK_KP_SLASH;
				res = true;
				state = 0;
				break;

			case 0x52:
				*c = VK_KP_MULTIPLY;
				res = true;
				state = 0;
				break;

			case 0x53:
				*c = VK_KP_MINUS;
				res = true;
				state = 0;
				break;

			default:
				state = 0;
				break;
			}
			break;

		case 3:
			switch (c_raw)
			{
			case 0x31:
				c_tmp = VK_HOME;
				state = 4;
				break;

			case 0x32:
				c_tmp = VK_INS;
				state = 5;
				break;

			case 0x34:
				c_tmp = VK_END;
				state = 6;
				break;

			case 0x35:
				c_tmp = VK_PGUP;
				state = 6;
				break;

			case 0x36:
				c_tmp = VK_PGDN;
				state = 6;
				break;

			case 0x41:
				*c = VK_UPARROW;
				res = true;
				state = 0;
				break;

			case 0x42:
				*c = VK_DOWNARROW;
				res = true;
				state = 0;
				break;

			case 0x43:
				*c = VK_RIGHTARROW;
				res = true;
				state = 0;
				break;

			case 0x44:
				*c = VK_LEFTARROW;
				res = true;
				state = 0;
				break;

			default:
				state = 0;
				break;
			}
			break;

		case 4:
			switch (c_raw)
			{
			case 0x31:
				c_tmp = VK_F1;
				state = 6;
				break;

			case 0x32:
				c_tmp = VK_F2;
				state = 6;
				break;

			case 0x33:
				c_tmp = VK_F3;
				state = 6;
				break;

			case 0x34:
				c_tmp = VK_F4;
				state = 6;
				break;

			case 0x35:
				c_tmp = VK_F5;
				state = 6;
				break;

			case 0x37:
				c_tmp = VK_F6;
				state = 6;
				break;

			case 0x38:
				c_tmp = VK_F7;
				state = 6;
				break;

			case 0x39:
				c_tmp = VK_F8;
				state = 6;
				break;

			case 0x7E:
				*c = c_tmp;
				res = true;
				state = 0;
				break;

			default:
				state = 0;
				break;
			}
			break;

		case 5:
			switch (c_raw)
			{
			case 0x30:
				c_tmp = VK_F9;
				state = 6;
				break;

			case 0x31:
				c_tmp = VK_F10;
				state = 6;
				break;

			case 0x33:
				c_tmp = VK_F11;
				state = 6;
				break;

			case 0x34:
				c_tmp = VK_F12;
				state = 6;
				break;

			case 0x7E:
				*c = c_tmp;
				res = true;
				state = 0;
				break;

			default:
				state = 0;
				break;
			}
			break;

		case 6:
			switch (c_raw)
			{
			case 0x7E:
				*c = c_tmp;
				res = true;
				state = 0;
				break;

			default:
				state = 0;
				break;
			}
			break;
		}
	}
	else if (timer_elapsed(tmr) && (state == 1))
	{
		*c = VK_ESC;
		res = true;
		state = 0;
	}
	return (res);
}

/*!
 * \brief This function compares two strings.
 *
 * \param s1 A pointer to a string.
 * \param s2 A pointer to a string.
 *
 * \return True if successful, false on error.
 */
static bool shell_strcmp(const char *s1, const char *s2)
{
	for (; *s1 == *s2; s1++, s2++)
	{
		if (*s1 == '\0')
			return (true);
	}

	if ((*s2 == ' ') || (*s2 == '\f') || (*s2 == '\n') || (*s2 == '\r') || (*s2 == '\t') || (*s2 == '\v'))
		return (true);

	return (false);
}

/*!
 * \brief This function initializes the command line object.
 *
 * \param object A pointer to a command line object.
 *
 * \return -.
 */
static void shell_cmdline_init(cmdline_obj_t *object)
{
	uint8_t i;

	for (i = 0; i < (SHELL_CMDLINE_HIST_COUNT + 1); i++)
	{
		object->cmdline[i].id = i;
		object->cmdline[i].str = &mem_pool[SHELL_CMDLINE_SIZE * i];
		object->cmdline[i].size = 0;

		if (object->cmdline[i].id == SHELL_CMDLINE_HIST_COUNT)
		{
			object->p_cmdline = &object->cmdline[i];
			object->p_cmdline->size = 0;
		}
	}
#if (SHELL_CMDLINE_HIST_COUNT > 0)
	object->cmd_idx = 0;
#endif
	object->tabline.str = NULL;
	object->tabline.size = 0;
	object->tab_idx = 0;
}

/*!
 * \brief This function adds an item to the command line object.
 *
 * \param object A pointer to a command line object.
 *
 * \return -.
 */
static void shell_cmdline_add(cmdline_obj_t *object)
{
	uint8_t i;

	for (i = 0; i < (SHELL_CMDLINE_HIST_COUNT + 1); i++)
	{
		object->cmdline[i].id++;
		if (object->cmdline[i].id > SHELL_CMDLINE_HIST_COUNT)
			object->cmdline[i].id = 0;

		if (object->cmdline[i].id == SHELL_CMDLINE_HIST_COUNT)
		{
			object->p_cmdline = &object->cmdline[i];
			object->p_cmdline->size = 0;
		}
	}
#if (SHELL_CMDLINE_HIST_COUNT > 0)
	object->cmd_idx = 0;
#endif
}

#if (SHELL_CMDLINE_HIST_COUNT > 0)
/*!
 * \brief This function retrieves an item from the command line object.
 *
 * \param object A pointer to a command line object.
 *
 * \return -.
 */
static void shell_cmdline_get(cmdline_obj_t *object)
{
	uint8_t i;

	for (i = 0; i < (SHELL_CMDLINE_HIST_COUNT + 1); i++)
	{
		if (object->cmdline[i].id == object->cmd_idx)
		{
			memcpy(object->p_cmdline->str, object->cmdline[i].str, object->cmdline[i].size);
			object->p_cmdline->size = object->cmdline[i].size;
			break;
		}
	}
}

/*!
 * \brief This function retrieves an item from the command line object.
 *
 * \param object A pointer to a command line object.
 *
 * \return -.
 */
static void shell_cmdline_get_prev(cmdline_obj_t *object)
{
	shell_cmdline_get(object);
	if (!object->p_cmdline->size)
	{
		object->cmd_idx--;
		shell_cmdline_get(object);
	}

	if (object->cmd_idx < (SHELL_CMDLINE_HIST_COUNT - 1))
	object->cmd_idx++;

}

/*!
 * \brief This function retrieves an item from the command line object.
 *
 * \param object A pointer to a command line object.
 *
 * \return -.
 */
static void shell_cmdline_get_next(cmdline_obj_t *object)
{
	if (object->p_cmdline->size)
	{
		if (object->cmd_idx > 0)
		object->cmd_idx--;

		shell_cmdline_get(object);
		if (!object->p_cmdline->size)
		{
			object->cmd_idx++;
			shell_cmdline_get(object);
		}
	}
}
#endif

static bool shell_cmdline_execute(cmdline_obj_t *object)
{
	uint8_t i;

	for (i = 0; i < CMD_TBL_COUNT; i++)
	{
		if (shell_strcmp(cmd_tbl[i].cmd, object->p_cmdline->str))
			break;
	}

	if (i < CMD_TBL_COUNT)
	{
		if (cmd_tbl[i].fxn)
		{
			char * p;
			p = strchr(cmdline.p_cmdline->str, ' ');
			cmd_tbl[i].fxn(p);
		}

		return (true);
	}

	return (false);
}

/*!
 * \brief This function finds a (partial) command in a table.
 *
 * \param object A pointer to a tab line object.
 *
 * \return -.
 */
static bool shell_tabline_find(cmdline_obj_t *object)
{
	uint8_t i;

	for (object->tabline.str = NULL, i = 0; (object->tabline.str == NULL) && (i < CMD_TBL_COUNT); i++)
	{
		if (!strncmp(cmd_tbl[object->tab_idx].cmd, object->p_cmdline->str, object->p_cmdline->size))
		{
			object->tabline.str = (char *) cmd_tbl[object->tab_idx].cmd;
			object->tabline.size = strlen(object->tabline.str);
		}

		object->tab_idx++;
		if (object->tab_idx >= CMD_TBL_COUNT)
			object->tab_idx = 0;
	}

	return (object->tabline.str != NULL);
}

/*!
 * \brief This function copies the last tab line to the command line.
 *
 * \param object A pointer to a tab line object.
 *
 * \return -.
 */
static void shell_tabline_get(cmdline_obj_t *object)
{
	if (object->tabline.str)
	{
		memcpy(object->p_cmdline->str, object->tabline.str, object->tabline.size);
		object->p_cmdline->size = object->tabline.size;

		object->tabline.str = NULL;
	}
	object->tab_idx = 0;
}

/*
 * ---------------------------------- Custom Shell functions ----------------------------------
 */

/*!
 * \brief This function handles the shell help command.
 *
 * \param -.
 *
 * \return -.
 */
static void sh_help(char *param)
{
	uint8_t i;

	HAL_StatusTypeDef err;

	tty_puts("No Help\r\n");

}

/*!
 * \brief
 *
 * \param
 *
 * \return -.
 */
void sh_ver(char *param)
{
#if defined CHARGER_VERSION
	tty_puts("MT' (@) 2019 STM32L0 CHG_BL\r\n");
#endif
#if defined BATTERY_VERSION
	tty_puts("MT' (@) 2019 STM32L0 BAT_BL\r\n");

#endif

}

/*!
 * \brief
 *
 * \param
 *
 * \return -.
 */
static void sh_reboot(char *param)
{
	NVIC_SystemReset();
}

/*!
 * \brief
 *
 * \param
 *
 * \return -.
 */
static void sh_ea(char *param)
{
	char buf[10];

	tty_puts("Erasing application flash ");
	tty_puts(my_itoa(APP_SIZE / 1024, buf, 10));
	tty_puts("Kb");
	if (stm32_flash_erase(FLASH_APP_START_ADDR, APP_SIZE - 1) != HAL_OK)
		tty_puts("ERROR\r\n");
	else
		tty_puts(" OK\r\n");
}


/*!
 * \brief
 *
 * \param
 *
 * \return -.
 */
static void sh_ua(char *param)
{
	tty_puts("Send .bin Ymodem\r\n");
	ymodem_receive(FLASH_APP_START_ADDR, APP_SIZE, stm32_ymodem_flash_write);
	tty_puts("\r\nOK\r\n");

}



/*!
 * \brief
 *
 * \param
 *
 * \return -.
 */
static void sh_vi(char *param)
{
	extern app_info_t c_app_info;
	char buf[10];
#if 1
	app_info_t *p_app_info = (app_info_t*) FLASH_APP_START_ADDR;
	app_info_t shadow_info;

	tty_puts("sci2 STM32 bootloader ");

	tty_puts("STM32 bootloader ");
	tty_puts(my_itoa((c_app_info.version >> 24) & 0xFF, buf, 16));
	tty_puts(".");
	tty_puts(my_itoa((c_app_info.version >> 16) & 0xFF, buf, 16));
	tty_puts(" (");
	tty_puts(__DATE__);
	tty_puts(" ");
	tty_puts(__TIME__);
	tty_puts(")\r\n");

	if (p_app_info->key == APP_KEY)
	{
		tty_puts("Loaded Application: ");
		tty_puts(my_itoa((p_app_info->version >> 24) & 0xFF, buf, 16));
		tty_puts(".");
		tty_puts(my_itoa((p_app_info->version >> 16) & 0xFF, buf, 16));
		tty_puts(".");
		tty_puts(my_itoa((p_app_info->version >> 8) & 0xFF, buf, 16));
		tty_puts(" (");
		tty_puts(p_app_info->build_date);
		tty_puts(" ");
		tty_puts(p_app_info->build_time);
		tty_puts(")\r\n");
	}
	else
		tty_puts("No Application\r\n");

#endif
}

/*!
 * \brief
 *
 * \param
 *
 * \return -.
 */
static void sh_st(char *param)
{
	app_info_t app_info;
	app_info_t *p_app_info = (app_info_t*) FLASH_APP_START_ADDR;

	if (p_app_info->key == APP_KEY)
	{
		// Verify integrity.
		if (crc_verify_flash(&app_info, FLASH_APP_START_ADDR, FLASH_APP_END_ADDR - FLASH_APP_START_ADDR) == 0)
		{
			tty_puts("<Start application>\r\n");
			HAL_Delay(200);
			JumptoApp();
		}
		else
		{
			tty_puts("APP CRC error\r\n");
		}

	}
	else
		tty_puts("No application\r\n");

}

/*!
 * \brief
 *
 * \param
 *
 * \return -.
 */
void sh_crc(char *param)
{
	app_info_t app_info;

	app_info_t *p_app_info = (app_info_t*) FLASH_APP_START_ADDR;

	if (p_app_info->key == APP_KEY)
	{
		// Verify integrity.
		if (crc_verify_flash(&app_info, FLASH_APP_START_ADDR, APP_SIZE) == 0)
			tty_puts("APP CRC ok\r\n");
		else
			tty_puts("APP CRC error\r\n");

	}
	else
		tty_puts("No application\r\n");
}




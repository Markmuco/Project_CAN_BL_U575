/*
 * uart_sci.c
 *
 *  Created on: 11 dec. 2020
 *      Author: Mark Ursum


 DESCRIPTION: Serial Queue driver for STM32:
 defined (STM32L4)  || defined (STM32F0) || defined (STM32WB) || defined (STM32F3)
 defined (STM32F4) || defined (STM32F1)
 defined (STM32G0) || defined (STM32G4) || defined (STM3C0)|| defined (STM32U0) || defined (STM32U5)
 Place the in the _it file:
	sci1_callback()
 start the unit with:
	init_sci();
 
 1.00
 - inital version
 1.01
 - 
 1.02
 -
 1.03
 - no more use of byte counter
 1.04
 - added USB support
 1.05
 - added and tested U083
 1.06
  - RS485 using TC Transmission complete IRQ
 1.07
  - added USB CDC Host (also need usb_host.c/.h)
 1.08
 - added U575 support

 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "main.h"
#include "usart.h"
#include "uart_sci.h"

#if FLEXIBLE_CAN
#include "can_functions.h"
#include "canq.h"
#endif
#if (SCI_CFG_USB_INCLUDED)
#if (SCI_USB_DEVICE)
#include "usbd_cdc_if.h"
#else
#include "usbh_cdc.h"
#include "usb_host.h"
#endif
#endif
static bool sci_init(sci_ch_t ch, void (*const p_callback)(void *p_args));
static bool sci_close(sci_ch_t ch);
static bool sci_putc(sci_ch_t ch, uint8_t c);
static bool sci_puts(sci_ch_t ch, uint8_t *str);
static bool sci_putsn(sci_ch_t ch, uint8_t *str, uint16_t len);
static bool sci_getc(sci_ch_t ch, uint8_t *c);
static bool rx_queue_busy(sci_ch_t ch);
static bool tx_queue_busy(sci_ch_t ch);
#if 0
static bool sci_in_waiting(sci_ch_t ch, size_t *cnt);
static bool sci_out_waiting(sci_ch_t ch, size_t *cnt);
#endif
static void sci_enable_stop_mode(sci_ch_t ch);
static void sci_disable_stop_mode(sci_ch_t ch);
static void isr_handler(const sci_hdl_t hdl);

#if FLEXIBLE_SCI1 || FLEXIBLE_SCI2 || FLEXIBLE_SCI3 || FLEXIBLE_SCI4 || FLEXIBLE_SCI5 || FLEXIBLE_SCI6 || FLEXIBLE_SCI7 || FLEXIBLE_SCI8 || FLEXIBLE_CAN
// pointer to active tty port
fp_t fp; /*!< Function pointer to SCI functions */
#endif

#if (SCI_CFG_CH1_INCLUDED)
static uint8_t ch1_tx_buf[1 << SCI_CFG_CH1_TX_BUFSIZ_ORDER];
static uint8_t ch1_rx_buf[1 << SCI_CFG_CH1_RX_BUFSIZ_ORDER];

static sci_queue_t ch1_tx_queue;
static sci_queue_t ch1_rx_queue;

static sci_ch_ctrl_t ch1_ctrl =
{
		.handle = &huart1,
		.tx_queue = &ch1_tx_queue,
		.rx_queue = &ch1_rx_queue,
		.callback = NULL };
#endif

#if (SCI_CFG_CH2_INCLUDED)
static uint8_t ch2_tx_buf[1 << SCI_CFG_CH2_TX_BUFSIZ_ORDER];
static uint8_t ch2_rx_buf[1 << SCI_CFG_CH2_RX_BUFSIZ_ORDER];

static sci_queue_t ch2_tx_queue;
static sci_queue_t ch2_rx_queue;

static sci_ch_ctrl_t ch2_ctrl =
{
		.handle = &huart2,
		.tx_queue = &ch2_tx_queue,
		.rx_queue = &ch2_rx_queue,
		.callback = NULL };
#endif

#if (SCI_CFG_CH3_INCLUDED)
static uint8_t ch3_tx_buf[1 << SCI_CFG_CH3_TX_BUFSIZ_ORDER];
static uint8_t ch3_rx_buf[1 << SCI_CFG_CH3_RX_BUFSIZ_ORDER];

static sci_queue_t ch3_tx_queue;
static sci_queue_t ch3_rx_queue;

static sci_ch_ctrl_t ch3_ctrl =
{
	.handle = &huart3,
	.tx_queue = &ch3_tx_queue,
	.rx_queue = &ch3_rx_queue,
	.callback = NULL
};
#endif

#if (SCI_CFG_CH4_INCLUDED)
static uint8_t ch4_tx_buf[1 << SCI_CFG_CH4_TX_BUFSIZ_ORDER];
static uint8_t ch4_rx_buf[1 << SCI_CFG_CH4_RX_BUFSIZ_ORDER];

static sci_queue_t ch4_tx_queue;
static sci_queue_t ch4_rx_queue;

static sci_ch_ctrl_t ch4_ctrl =
{
	.handle = &huart4,
	.tx_queue = &ch4_tx_queue,
	.rx_queue = &ch4_rx_queue,
	.callback = NULL
};
#endif

#if (SCI_CFG_CH5_INCLUDED)
static uint8_t ch5_tx_buf[1 << SCI_CFG_CH5_TX_BUFSIZ_ORDER];
static uint8_t ch5_rx_buf[1 << SCI_CFG_CH5_RX_BUFSIZ_ORDER];

static sci_queue_t ch5_tx_queue;
static sci_queue_t ch5_rx_queue;

static sci_ch_ctrl_t ch5_ctrl =
{
	.handle = &huart5,
	.tx_queue = &ch5_tx_queue,
	.rx_queue = &ch5_rx_queue,
	.callback = NULL
};
#endif

#if (SCI_CFG_CH6_INCLUDED)
static uint8_t ch6_tx_buf[1 << SCI_CFG_CH6_TX_BUFSIZ_ORDER];
static uint8_t ch6_rx_buf[1 << SCI_CFG_CH6_RX_BUFSIZ_ORDER];

static sci_queue_t ch6_tx_queue;
static sci_queue_t ch6_rx_queue;

static sci_ch_ctrl_t ch6_ctrl =
{
	.handle = &huart6,
	.tx_queue = &ch6_tx_queue,
	.rx_queue = &ch6_rx_queue,
	.callback = NULL
};
#endif

#if (SCI_CFG_CH7_INCLUDED)
static uint8_t ch7_tx_buf[1 << SCI_CFG_CH7_TX_BUFSIZ_ORDER;
static uint8_t ch7_rx_buf[1 << SCI_CFG_CH7_RX_BUFSIZ_ORDER];

static sci_queue_t ch7_tx_queue;
static sci_queue_t ch7_rx_queue;

static sci_ch_ctrl_t ch7_ctrl =
{
	.handle = &huart7,
	.tx_queue = &ch7_tx_queue,
	.rx_queue = &ch7_rx_queue,
	.callback = NULL
};
#endif

#if (SCI_CFG_CH8_INCLUDED)
static uint8_t ch8_tx_buf[1 << SCI_CFG_CH8_TX_BUFSIZ_ORDER];
static uint8_t ch8_rx_buf[1 << SCI_CFG_CH8_RX_BUFSIZ_ORDER];

static sci_queue_t ch8_tx_queue;
static sci_queue_t ch8_rx_queue;

static sci_ch_ctrl_t ch8_ctrl =
{
	.handle = &huart8,
	.tx_queue = &ch8_tx_queue,
	.rx_queue = &ch8_rx_queue,
	.callback = NULL
};
#endif

#if (SCI_CFG_CH9_INCLUDED)
static uint8_t ch9_tx_buf[1 << SCI_CFG_CH9_TX_BUFSIZ_ORDER];
static uint8_t ch9_rx_buf[1 << SCI_CFG_CH9_RX_BUFSIZ_ORDER];

static sci_queue_t ch9_tx_queue;
static sci_queue_t ch9_rx_queue;

static sci_ch_ctrl_t ch9_ctrl =
{
	.handle = &huart9,
	.tx_queue = &ch9_tx_queue,
	.rx_queue = &ch9_rx_queue,
	.callback = NULL
};
#endif

#if (SCI_CFG_CAN_INCLUDED)
static uint8_t can_tx_buf[1 << SCI_CFG_CAN_TX_BUFSIZ_ORDER];
static uint8_t can_rx_buf[1 << SCI_CFG_CAN_RX_BUFSIZ_ORDER];

static sci_queue_t can_tx_queue;
static sci_queue_t can_rx_queue;

static sci_ch_ctrl_t can_ctrl =
{
		.handle = NULL,
		.tx_queue = &can_tx_queue,
		.rx_queue = &can_rx_queue,
		.callback = NULL
};
#endif

#if (SCI_CFG_LPCH1_INCLUDED)
static uint8_t lpch1_tx_buf[1 << SCI_CFG_LPCH1_TX_BUFSIZ_ORDER];
static uint8_t lpch1_rx_buf[1 << SCI_CFG_LPCH1_RX_BUFSIZ_ORDER];

static sci_queue_t lpch1_tx_queue;
static sci_queue_t lpch1_rx_queue;

static sci_ch_ctrl_t lpch1_ctrl =
{
		.handle = &hlpuart1,
		.tx_queue = &lpch1_tx_queue,
		.rx_queue = &lpch1_rx_queue,
		.callback = NULL
};
#endif

#if (SCI_CFG_USB_INCLUDED)
static uint8_t usb_tx_buf[1 << SCI_CFG_USB_TX_BUFSIZ_ORDER];
static uint8_t usb_rx_buf[1 << SCI_CFG_USB_RX_BUFSIZ_ORDER];

static sci_queue_t usb_tx_queue;
static sci_queue_t usb_rx_queue;
static bool usb_tx_idle = true;
static sci_ch_ctrl_t usb_ctrl =
{
	.tx_queue = &usb_tx_queue,
	.rx_queue = &usb_rx_queue,
	.callback = NULL
};
#endif

static const sci_hdl_t g_handles[SCI_NUM_CH] =
{
#if SCI_CFG_CH1_INCLUDED
		&ch1_ctrl,
#else
		NULL,
#endif
#if SCI_CFG_CH2_INCLUDED
		&ch2_ctrl,
#else
		NULL,
#endif
#if SCI_CFG_CH3_INCLUDED
		&ch3_ctrl,
#else
		NULL,
#endif
#if SCI_CFG_CH4_INCLUDED
		&ch4_ctrl,
#else
		NULL,
#endif
#if SCI_CFG_CH5_INCLUDED
		&ch5_ctrl,
#else
		NULL,
#endif
#if SCI_CFG_CH6_INCLUDED
		&ch6_ctrl,
#else
		NULL,
#endif
#if SCI_CFG_CH7_INCLUDED
		&ch7_ctrl,
#else
		NULL,
#endif
#if SCI_CFG_CH8_INCLUDED
		&ch8_ctrl,
#else
		NULL,
#endif
#if SCI_CFG_CH9_INCLUDED
		&ch9_ctrl,
#else
		NULL,
#endif
#if SCI_CFG_LPCH1_INCLUDED
		&lpch1_ctrl,
#else
		NULL,
#endif
#if SCI_CFG_CAN_INCLUDED
		&can_ctrl,
#else
		NULL,
#endif
#if SCI_CFG_USB_INCLUDED
		&usb_ctrl,
#else
		NULL
#endif
		};

#if 0
/*!
 * \brief This function is called by printf
 *
 * \param -
 *
 * \return -
 */
void _write(int file, char *ptr, int len)
{
	int i;
	for (i = 0; i < len; i++)
	{
		HAL_UART_Transmit(&huart1, (uint8_t*) &ptr[i], 1, 10);
	}
}
#endif
/*
 * Callback on receiving char
 */
static void f_sci1(void *p_args)
{
#if 0
	static uint8_t esc = 0;
//	volatile int *var = (int*) RAM_KEY;
  sci_cb_args_t *args = (sci_cb_args_t *)p_args;

	if (args->byte == ESC)
	{
		if (esc++ == 10)
		{
			esc = 0;
			tty_puts("SCI1\r\n");
			shell_use_sci1();
		}
	}
	else
		esc = 0;
#endif
}



/*
 * Init all usarts
 */
void init_sci(void)
{
#if (SCI_CFG_CH1_INCLUDED)
	sci_init(SCI_CH1, f_sci1);
#endif
#if (SCI_CFG_CH2_INCLUDED)
	sci_init(SCI_CH2, f_sci2);
#endif
#if (SCI_CFG_CH3_INCLUDED)
	sci_init(SCI_CH3, NULL);
#endif
#if (SCI_CFG_CH4_INCLUDED)
	sci_init(SCI_CH4, NULL);
#endif
#if (SCI_CFG_CH5_INCLUDED)
	sci_init(SCI_CH5, NULL);
#endif
#if (SCI_CFG_CH6_INCLUDED)
	sci_init(SCI_CH6, NULL);
#endif
#if (SCI_CFG_CH7_INCLUDED)
	sci_init(SCI_CH7, NULL);
#endif
#if (SCI_CFG_CH8_INCLUDED)
	sci_init(SCI_CH8, NULL);
#endif
#if (SCI_CFG_CH9_INCLUDED)
	sci_init(SCI_CH9, NULL);
#endif
#if (SCI_CFG_LPCH1_INCLUDED)
	sci_init(SCI_LPCH1, NULL);
#endif
#if (SCI_CFG_CAN_INCLUDED)
	sci_init(SCI_CAN, NULL);
#endif
#if (SCI_CFG_USB_INCLUDED)
	sci_init(SCI_USB, NULL);
#endif

}
/*
 * ******************************* SCI-1 *******************************
 */
#if (SCI_CFG_CH1_INCLUDED)
void sci1_callback(void)
{
	isr_handler(&ch1_ctrl);
}
bool sci1_getch(char *c)
{
	return (sci_getc(SCI_CH1, (uint8_t *) c));
}
bool sci1_putc(char c)
{
	return (sci_putc(SCI_CH1, c));
}
bool sci1_puts(char *str)
{
	return (sci_puts(SCI_CH1, (uint8_t *) str));
}
bool sci1_putsn(char *str, uint16_t len)
{
	return (sci_putsn(SCI_CH1, (uint8_t *) str, len));
}
#if USE_RS485>0
bool sci1_rxq_busy(void)
{
	return (rx_queue_busy(SCI_CH1));
}
bool sci1_txq_busy(void)
{
	return (tx_queue_busy(SCI_CH1));
}
#endif
bool sci1_printf(const char *format, ...)
{
	char str[255 + 1];
	va_list args;

	va_start(args, format);
	vsnprintf(str, sizeof(str), format, args);
	va_end(args);

	return (sci_puts(SCI_CH1, (uint8_t *) str));
}
#if FLEXIBLE_SCI1
void shell_use_sci1(void)
{
	fp.sci_printf = *sci1_printf;
	fp.sci_putc = *sci1_putc;
	fp.sci_puts = *sci1_puts;
	fp.sci_putsn = *sci1_putsn;
	fp.sci_getch = *sci1_getch;
}
#endif
#endif
/*
 * ******************************* SCI-2 *******************************
 */
#if (SCI_CFG_CH2_INCLUDED)
void sci2_callback(void)
{
	isr_handler(&ch2_ctrl);
}
bool sci2_getch(char *c)
{
	return (sci_getc(SCI_CH2, (uint8_t*) c));
}
bool sci2_putc(char c)
{
	return (sci_putc(SCI_CH2, c));
}
bool sci2_puts(char *str)
{
	return (sci_puts(SCI_CH2, (uint8_t*) str));
}
bool sci2_putsn(char *str, uint16_t len)
{
	return (sci_putsn(SCI_CH2, (uint8_t*) str, len));
}
#if USE_RS485>0
bool sci2_rxq_busy(void)
{
	return (rx_queue_busy(SCI_CH2));
}
bool sci2_txq_busy(void)
{
	return (tx_queue_busy(SCI_CH2));
}
#endif
bool sci2_printf(const char *format, ...)
{
	char str[255 + 1];
	va_list args;

	va_start(args, format);
	vsnprintf(str, sizeof(str), format, args);
	va_end(args);

	return (sci_puts(SCI_CH2, (uint8_t*) str));
}
#if FLEXIBLE_SCI2
void shell_use_sci2(void)
{
	fp.sci_printf = *sci2_printf;
	fp.sci_putc = *sci2_putc;
	fp.sci_puts = *sci2_puts;
	fp.sci_putsn = *sci2_putsn;
	fp.sci_getch = *sci2_getch;
}
#endif
#endif
/* ******************************* SCI-3 *******************************
 */
#if (SCI_CFG_CH3_INCLUDED)
void sci3_callback(void)
{
	isr_handler(&ch3_ctrl);
}
bool sci3_getch(char *c)
{
	return (sci_getc(SCI_CH3, (uint8_t *)c));
}
bool sci3_putc(char c)
{
	return (sci_putc(SCI_CH3, c));
}
bool sci3_puts(char *str)
{
	return (sci_puts(SCI_CH3, (uint8_t *)str));
}
bool sci3_putsn(char *str, uint16_t len)
{
	return (sci_putsn(SCI_CH3, (uint8_t *)str, len));
}
#if USE_RS485>0
bool sci3_rxq_busy(void)
{
	return (rx_queue_busy(SCI_CH3));
}
bool sci3_txq_busy(void)
{
	return (tx_queue_busy(SCI_CH3));
}
#endif
bool sci3_printf(char *format, ...)
{
	char str[255+1];
	va_list args;

	va_start(args, format);
	vsnprintf(str, sizeof(str), format, args);
	va_end(args);

	return (sci_puts(SCI_CH3, (uint8_t *)str));
}
#if FLEXIBLE_SCI3
void shell_use_sci3(void)
{
	fp.sci_printf = *sci3_printf;
	fp.sci_putc = *sci3_putc;
	fp.sci_puts = *sci3_puts;
	fp.sci_putsn = *sci3_putsn;
	fp.sci_getch = *sci3_getch;
}
#endif
#endif
/* ******************************* SCI-4 *******************************
 */
#if (SCI_CFG_CH4_INCLUDED)
void sci4_callback(void)
{
	isr_handler(&ch4_ctrl);
}
bool sci4_getch(char *c)
{
	return (sci_getc(SCI_CH4, (uint8_t *)c));
}
bool sci4_putc(char c)
{
	return (sci_putc(SCI_CH4, c));
}
bool sci4_puts(char *str)
{
	return (sci_puts(SCI_CH4, (uint8_t *)str));
}
bool sci4_putsn(char *str, uint16_t len)
{
	return (sci_putsn(SCI_CH4, (uint8_t *)str, len));
}
#if USE_RS485>0
bool sci4_rxq_busy(void)
{
	return (rx_queue_busy(SCI_CH4));
}
bool sci4_txq_busy(void)
{
	return (tx_queue_busy(SCI_CH4));
}
#endif
bool sci4_printf(char *format, ...)
{
	char str[255+1];
	va_list args;

	va_start(args, format);
	vsnprintf(str, sizeof(str), format, args);
	va_end(args);

	return (sci_puts(SCI_CH4, (uint8_t *)str));
}
#if FLEXIBLE_SCI4
void shell_use_sci4(void)
{
	fp.sci_printf = *sci4_printf;
	fp.sci_putc = *sci4_putc;
	fp.sci_puts = *sci4_puts;
	fp.sci_putsn = *sci4_putsn;
	fp.sci_getch = *sci4_getch;
}
#endif
#endif
/* ******************************* SCI-5 *******************************
 */
#if (SCI_CFG_CH5_INCLUDED)
void sci5_callback(void)
{
	isr_handler(&ch5_ctrl);
}
bool sci5_getch(char *c)
{
	return (sci_getc(SCI_CH5, (uint8_t *)c));
}
bool sci5_putc(char c)
{
	return (sci_putc(SCI_CH5, c));
}
bool sci5_puts(char *str)
{
	return (sci_puts(SCI_CH5, (uint8_t *)str));
}
bool sci5_putsn(char *str, uint16_t len)
{
	return (sci_putsn(SCI_CH5, (uint8_t *)str, len));
}
#if USE_RS485>0
bool sci5_rxq_busy(void)
{
	return (rx_queue_busy(SCI_CH5));
}
bool sci5_txq_busy(void)
{
	return (tx_queue_busy(SCI_CH5));
}
#endif
bool sci5_printf(char *format, ...)
{
	char str[255+1];
	va_list args;

	va_start(args, format);
	vsnprintf(str, sizeof(str), format, args);
	va_end(args);

	return (sci_puts(SCI_CH5, (uint8_t *)str));
}
#if FLEXIBLE_SCI5
void shell_use_sci5(void)
{
	fp.sci_printf = *sci5_printf;
	fp.sci_putc = *sci5_putc;
	fp.sci_puts = *sci5_puts;
	fp.sci_putsn = *sci5_putsn;
	fp.sci_getch = *sci5_getch;
}
#endif
#endif
/* ******************************* SCI-6 *******************************
 */
#if (SCI_CFG_CH6_INCLUDED)
void sci6_callback(void)
{
	isr_handler(&ch6_ctrl);
}
bool sci6_getch(char *c)
{
	return (sci_getc(SCI_CH6, (uint8_t *)c));
}
bool sci6_putc(char c)
{
	return (sci_putc(SCI_CH6, c));
}
bool sci6_puts(char *str)
{
	return (sci_puts(SCI_CH6, (uint8_t *)str));
}
bool sci6_putsn(char *str, uint16_t len)
{
	return (sci_putsn(SCI_CH6, (uint8_t *)str, len));
}
#if USE_RS485>0
bool sci6_rxq_busy(void)
{
	return (rx_queue_busy(SCI_CH6));
}
bool sci6_txq_busy(void)
{
	return (tx_queue_busy(SCI_CH6));
}
#endif
bool sci6_printf(char *format, ...)
{
	char str[255+1];
	va_list args;

	va_start(args, format);
	vsnprintf(str, sizeof(str), format, args);
	va_end(args);

	return (sci_puts(SCI_CH6, (uint8_t *)str));
}
#if FLEXIBLE_SCI6
void shell_use_sci6(void)
{
	fp.sci_printf = *sci6_printf;
	fp.sci_putc = *sci6_putc;
	fp.sci_puts = *sci6_puts;
	fp.sci_putsn = *sci6_putsn;
	fp.sci_getch = *sci6_getch;
}
#endif
#endif
/* ******************************* SCI-7 *******************************
 */
#if (SCI_CFG_CH7_INCLUDED)
void sci7_callback(void)
{
	isr_handler(&ch7_ctrl);
}
bool sci7_getch(char *c)
{
	return (sci_getc(SCI_CH7, (uint8_t *)c));
}
bool sci7_putc(char c)
{
	return (sci_putc(SCI_CH7, c));
}
bool sci7_puts(char *str)
{
	return (sci_puts(SCI_CH7, (uint8_t *)str));
}
bool sci7_putsn(char *str, uint16_t len)
{
	return (sci_putsn(SCI_CH7, (uint8_t *)str, len));
}
#if USE_RS485>0
bool sci7_rxq_busy(void)
{
	return (rx_queue_busy(SCI_CH7));
}
bool sci7_txq_busy(void)
{
	return (tx_queue_busy(SCI_CH7));
}
#endif
bool sci7_printf(char *format, ...)
{
	char str[255+1];
	va_list args;

	va_start(args, format);
	vsnprintf(str, sizeof(str), format, args);
	va_end(args);

	return (sci_puts(SCI_CH7, (uint8_t *)str));
}
#if FLEXIBLE_SCI7
void shell_use_sci7(void)
{
	fp.sci_printf = *sci7_printf;
	fp.sci_putc = *sci7_putc;
	fp.sci_puts = *sci7_puts;
	fp.sci_putsn = *sci7_putsn;
	fp.sci_getch = *sci7_getch;
}
#endif
#endif
/* ******************************* SCI-8 *******************************
 */
#if (SCI_CFG_CH8_INCLUDED)
void sci8_callback(void)
{
	isr_handler(&ch8_ctrl);
}
bool sci8_getch(char *c)
{
	return (sci_getc(SCI_CH8, (uint8_t *)c));
}
bool sci8_putc(char c)
{
	return (sci_putc(SCI_CH8, c));
}
bool sci8_puts(char *str)
{
	return (sci_puts(SCI_CH8, (uint8_t *)str));
}
bool sci8_putsn(char *str, uint16_t len)
{
	return (sci_putsn(SCI_CH8, (uint8_t *)str, len));
}
#if USE_RS485>0
bool sci8_rxq_busy(void)
{
	return (rx_queue_busy(SCI_CH8));
}
bool sci8_txq_busy(void)
{
	return (tx_queue_busy(SCI_CH8));
}
#endif
bool sci8_printf(char *format, ...)
{
	char str[255+1];
	va_list args;

	va_start(args, format);
	vsnprintf(str, sizeof(str), format, args);
	va_end(args);

	return (sci_puts(SCI_CH8, (uint8_t *)str));
}
#if FLEXIBLE_SCI8
void shell_use_sci8(void)
{
	fp.sci_printf = *sci8_printf;
	fp.sci_putc = *sci8_putc;
	fp.sci_puts = *sci8_puts;
	fp.sci_putsn = *sci8_putsn;
	fp.sci_getch = *sci8_getch;
	fp.sci_rxq_busy = *sci8_rxq_busy;
	fp.sci_txq_busy = *sci8_txq_busy;
}
#endif
#endif
/* ******************************* SCI-9 *******************************
 */
#if (SCI_CFG_CH9_INCLUDED)
void sci9_callback(void)
{
	isr_handler(&ch9_ctrl);
}
bool sci9_getch(char *c)
{
	return (sci_getc(SCI_CH9, (uint8_t *)c));
}
bool sci9_putc(char c)
{
	return (sci_putc(SCI_CH9, c));
}
bool sci9_puts(char *str)
{
	return (sci_puts(SCI_CH9, (uint8_t *)str));
}
bool sci9_putsn(char *str, uint16_t len)
{
	return (sci_putsn(SCI_CH9, (uint8_t *)str, len));
}
#if USE_RS485>0
bool sci9_rxq_busy(void)
{
	return (rx_queue_busy(SCI_CH9));
}
bool sci9_txq_busy(void)
{
	return (tx_queue_busy(SCI_CH9));
}
#endif
bool sci9_printf(char *format, ...)
{
	char str[255+1];
	va_list args;

	va_start(args, format);
	vsnprintf(str, sizeof(str), format, args);
	va_end(args);

	return (sci_puts(SCI_CH9, (uint8_t *)str));
}
#if FLEXIBLE_SCI9
void shell_use_sci9(void)
{
	fp.sci_printf = *sci9_printf;
	fp.sci_putc = *sci9_putc;
	fp.sci_puts = *sci9_puts;
	fp.sci_putsn = *sci9_putsn;
	fp.sci_getch = *sci9_getch;
}
#endif
#endif

/* ******************************* SCI-LPCH1 *******************************
 */
#if (SCI_CFG_LPCH1_INCLUDED)
void sci_lpch1_callback(void)
{
	isr_handler(&lpch1_ctrl);
}
bool sci_lpch1_getch(char *c)
{
	return (sci_getc(SCI_LPCH1, (uint8_t*) c));
}
bool sci_lpch1_putc(char c)
{
	return (sci_putc(SCI_LPCH1, c));
}
bool sci_lpch1_puts(char *str)
{
	return (sci_puts(SCI_LPCH1, (uint8_t*) str));
}
bool sci_lpch1_putsn(char *str, uint16_t len)
{
	return (sci_putsn(SCI_LPCH1, (uint8_t*) str, len));
}
bool sci_lpch1_printf(char *format, ...)
{
	char str[255 + 1];
	va_list args;

	va_start(args, format);
	vsnprintf(str, sizeof(str), format, args);
	va_end(args);

	return (sci_puts(SCI_LPCH1, (uint8_t*) str));
}
#if FLEXIBLE_SCI_LPCH1
void shell_use_sci_lpch1(void)
{
	fp.sci_printf = *sci_lpch1_printf;
	fp.sci_putc = *sci_lpch1_putc;
	fp.sci_puts = *sci_lpch1_puts;
	fp.sci_putsn = *sci_lpch1_putsn;
	fp.sci_getch = *sci_lpch1_getch;
}
#endif
#endif

/* ******************************* SCI-CAN *******************************
 */
#if (SCI_CFG_CAN_INCLUDED)
bool sci_can_getch(char *c)
{
	return (sci_getc(SCI_CAN, (uint8_t *) c));
}

bool sci_can_keyboard(char c) // Keyboard input comes from CAN bus
{
	sci_hdl_t hdl = g_handles[SCI_CAN];

	if ((hdl->rx_queue->in_index - hdl->rx_queue->out_index) <= hdl->rx_queue->size_mask)
    {
      hdl->rx_queue->buf[hdl->rx_queue->in_index & hdl->rx_queue->size_mask] = c;
      __DMB();
      hdl->rx_queue->in_index++;
    }
	else
		return false;

	return true;
}

bool sci_can_putc(char c)
{
	// send CAN data packet 8 bytes or rest
	return can_send_msg(ID_SH_TX, 1, (uint8_t *) &c, CAN_RTR_DATA);
}

bool sci_can_puts(char *str)
{
	int n = strlen(str);
	uint16_t free;
	int a;

	for (a = 0; a < (n / CAN_DATASIZE); a++)
	{
		free = can_tx_free();
		if (free)
			can_send_msg(ID_SH_TX, CAN_DATASIZE,(uint8_t *) &str[a * CAN_DATASIZE], CAN_RTR_DATA);
	}

	free = can_tx_free();
	if (free)
		can_send_msg(ID_SH_TX, (n % CAN_DATASIZE),(uint8_t *) &str[a * CAN_DATASIZE], CAN_RTR_DATA);
	return true;
}

bool sci_can_putsn(char *str, uint16_t len)
{
	uint16_t free;
	int a;

	for (a = 0; a < (len / CAN_DATASIZE); a++)
	{
		free = can_tx_free();
		if (free)
			can_send_msg(ID_SH_TX, CAN_DATASIZE,(uint8_t *) &str[a * CAN_DATASIZE], CAN_RTR_DATA);
	}

	free = can_tx_free();
	if (free)
		can_send_msg(ID_SH_TX, (len % CAN_DATASIZE),(uint8_t *) &str[a * CAN_DATASIZE], CAN_RTR_DATA);
	return true;
}

bool sci_can_printf(const char *format, ...)
{

	va_list ap;
	uint8_t buffer[S_BUF_SIZE];
	int n;
	int a = 0;
	uint16_t free;

	va_start(ap, format);
	n = vsnprintf((char*) buffer, S_BUF_SIZE, format, ap);
	va_end(ap);

	for (a = 0; a < (n / CAN_DATASIZE); a++)
	{
		free = can_tx_free();
		if (free)
			can_send_msg(ID_SH_TX, CAN_DATASIZE, &buffer[a * CAN_DATASIZE], CAN_RTR_DATA);
	}

	free = can_tx_free();
	if (free)
		can_send_msg(ID_SH_TX, (n % CAN_DATASIZE), &buffer[a * CAN_DATASIZE], CAN_RTR_DATA);
	return true;
}
#if USE_RS485>0
bool sci_can_rxq_busy(void)
{
	return (rx_queue_busy(SCI_CAN));
}
bool sci_can_txq_busy(void)
{
	return (can_tx_free() != CAN_TX_BUF_SIZE-1);
}
#endif
#if FLEXIBLE_CAN
void shell_use_can(void)
{
	fp.sci_printf = *sci_can_printf;
	fp.sci_putc = *sci_can_putc;
	fp.sci_puts = *sci_can_puts;
	fp.sci_putsn = *sci_can_putsn;
	fp.sci_getch = *sci_can_getch;
	fp.sci_rxq_busy = *sci_can_rxq_busy;
	fp.sci_txq_busy = *sci_can_txq_busy;
}
#endif
#endif

/* ******************************* SCI-USB *******************************
 */
#if (SCI_CFG_USB_INCLUDED)

// DEVICE: call from CDC_TransmitCplt_FS() usbd_cdc_if.c; send is complete
// HOST: call from  USBH_CDC_TransmitCallback() usbhost.c: 	if (Appli_state == APPLICATION_READY) sci_usb_callback();
void sci_usb_callback(void)
{
	// USB buffer is 64 bytes.
	static uint8_t buff[64]; // USB_HOST_Transmit needs a static buffer
	uint8_t cnt = 0;

	if (usb_tx_queue.in_index != usb_tx_queue.out_index)
	{
		while ((usb_tx_queue.in_index != usb_tx_queue.out_index) && cnt < 64)
		{
			buff[cnt++] = usb_tx_queue.buf[usb_tx_queue.out_index & usb_tx_queue.size_mask];
			__DMB();
			usb_tx_queue.out_index++;
		}
#if (SCI_USB_DEVICE)
		CDC_Transmit_FS(buff, cnt);
#else
		USB_HOST_Transmit(buff, cnt);
#endif
	}
	else
		usb_tx_idle = true;
}


// DEVICE: Call from CDC_Receive_FS() usbd_cdc_if.c char is received { sci_usb_rxdata(Buf, *Len); }
// HOST: Call from usb_host.c
/*
 */
void sci_usb_rxdata(uint8_t *Buf, uint32_t len)
{
	sci_hdl_t hdl = g_handles[SCI_USB];

	for (int var = 0; var < len; ++var)
	{
		if ((hdl->rx_queue->in_index - hdl->rx_queue->out_index) <= hdl->rx_queue->size_mask)
		{
			hdl->rx_queue->buf[hdl->rx_queue->in_index & hdl->rx_queue->size_mask] = Buf[var];
			__DMB();
			hdl->rx_queue->in_index++;
		}
	}
}

bool sci_usb_getch(char *c)
{
	return (sci_getc(SCI_USB, (uint8_t *)c));
}
bool sci_usb_putc(char c)
{
	return (sci_putc(SCI_USB, c));
}
bool sci_usb_puts(char *str)
{
	return (sci_puts(SCI_USB, (uint8_t *)str));
}
bool sci_usb_putsn(char *str, uint16_t len)
{
	return (sci_putsn(SCI_USB, (uint8_t *)str, len));
}

bool sci_usb_printf(const char *format, ...)
{
	char str[255 + 1];
	va_list args;

	va_start(args, format);
	vsnprintf(str, sizeof(str), format, args);
	va_end(args);

	return (sci_puts(SCI_USB, (uint8_t*) str));
}
#if FLEXIBLE_USB
void shell_use_usb(void)
{
	fp.sci_printf = *sci_usb_printf;
	fp.sci_putc = *sci_usb_putc;
	fp.sci_puts = *sci_usb_puts;
	fp.sci_putsn = *sci_usb_putsn;
	fp.sci_getch = *sci_usb_getch;
}
#endif
#endif

#if defined (STM32L4)  || defined (STM32F0) || defined (STM32WB) || defined (STM32F3)
#if (SCI_CFG_CH1_INCLUDED || SCI_CFG_CH2_INCLUDED || SCI_CFG_CH3_INCLUDED || SCI_CFG_CH4_INCLUDED || SCI_CFG_CH5_INCLUDED || SCI_CFG_CH6_INCLUDED || SCI_CFG_CH7_INCLUDED || SCI_CFG_CH8_INCLUDED)
/*!
 *
 * \brief -.
 *
 * \param hdl A handle to the SCI channel.
 *
 * \return -.
 */
static void isr_handler(const sci_hdl_t hdl)
{
	uint32_t isrflags = READ_REG(hdl->handle->Instance->ISR);
	uint32_t cr1its = READ_REG(hdl->handle->Instance->CR1);
	uint32_t errorflags = 0x00U;
#if defined (STM32U0)
	uint32_t cr3its     = READ_REG(hdl->handle->Instance->CR3);
#endif

	sci_cb_args_t args;
	uint8_t c;

	/* If no error occurs */
	errorflags = (isrflags & (uint32_t) (USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE | USART_ISR_RTOF));
	if (errorflags == RESET)
	{
	    /* UART in mode Receiver ---------------------------------------------------*/
		if (((isrflags & USART_ISR_RXNE) != RESET) && ((cr1its & USART_CR1_RX_EN) != RESET))
		{
			c = hdl->handle->Instance->RDR;

			if ((hdl->rx_queue->in_index - hdl->rx_queue->out_index) <= hdl->rx_queue->size_mask)
			{
				hdl->rx_queue->buf[hdl->rx_queue->in_index & hdl->rx_queue->size_mask] = c;
				__DMB();
				hdl->rx_queue->in_index++;
			}

			// handle the callback
			if (hdl->callback != NULL)
			{
				args.event = SCI_EVT_RX_CHAR;
				args.hdl = hdl;
				args.byte = c;
				hdl->callback((void*) &args);
			}
		}
		else
			args.event = SCI_EVT_RXBUF_OVFL;
	}

	if (__HAL_UART_GET_FLAG(hdl->handle, UART_FLAG_PE))
		__HAL_UART_CLEAR_PEFLAG(hdl->handle);

	if (__HAL_UART_GET_FLAG(hdl->handle, UART_FLAG_FE))
		__HAL_UART_CLEAR_FEFLAG(hdl->handle);

	if (__HAL_UART_GET_FLAG(hdl->handle, UART_FLAG_NE))
		__HAL_UART_CLEAR_NEFLAG(hdl->handle);

	if (__HAL_UART_GET_FLAG(hdl->handle, UART_FLAG_ORE))
		__HAL_UART_CLEAR_OREFLAG(hdl->handle);

	/* UART in mode Receiver --------------------------------------------------*/
	if (((isrflags & USART_ISR_TXE) != RESET) && ((cr1its & USART_CR1_TX_EN) != RESET))
	{
		if (hdl->tx_queue->in_index != hdl->tx_queue->out_index)
		{
			c = hdl->tx_queue->buf[hdl->tx_queue->out_index & hdl->tx_queue->size_mask];
			__DMB();
			hdl->tx_queue->out_index++;

			hdl->handle->Instance->TDR = c;
		}
		else
			CLEAR_BIT(hdl->handle->Instance->CR1, USART_CR1_TX_EN);
	}

	if (isrflags & USART_ISR_WUF)
	{
		// Clear Wakeup from Stop mode flag.
		SET_BIT(hdl->handle->Instance->ICR, UART_CLEAR_WUF);
	}

}
#endif
#endif

#if defined (STM32F4) || defined (STM32F1)
/*!
 *
 * \brief -.
 *
 * \param hdl A handle to the SCI channel.
 *
 * \return -.
 */
static void isr_handler(const sci_hdl_t hdl)
{
	uint32_t isrflags = READ_REG(hdl->handle->Instance->SR);
	uint32_t cr1its = READ_REG(hdl->handle->Instance->CR1);
	uint32_t errorflags = 0x00U;

	sci_cb_args_t args;
	uint8_t c;
#if USE_RS485>0
	// clear the RS485 transmitter send
	if (hdl->dir_port != NULL)
	{
		if ((isrflags & USART_SR_TC) != RESET)
		{
			HAL_GPIO_WritePin(hdl->dir_port, hdl->dir_pin, GPIO_PIN_RESET); // transmitter off
			CLEAR_BIT(hdl->handle->Instance->CR1, USART_CR1_TCIE); // Clear transmission complete
		}
	}
#endif
	/* If no error occurs */
	errorflags = (isrflags & (uint32_t) (USART_SR_PE | USART_SR_FE | USART_SR_ORE | USART_SR_NE));
	if (errorflags == RESET)
	{
		if (((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RX_EN) != RESET))
		{
			c = hdl->handle->Instance->DR;

			if ((hdl->rx_queue->in_index - hdl->rx_queue->out_index) <= hdl->rx_queue->size_mask)
			{
				hdl->rx_queue->buf[hdl->rx_queue->in_index & hdl->rx_queue->size_mask] = c;
				__DMB();
				hdl->rx_queue->in_index++;
			}

			// handle the callback
			if (hdl->callback != NULL)
			{
				args.event = SCI_EVT_RX_CHAR;
				args.hdl = hdl;
				args.byte = c;
				hdl->callback((void*) &args);
			}
		}
		else
		{
			args.event = SCI_EVT_RXBUF_OVFL;
		}
	}

	if (__HAL_UART_GET_FLAG(hdl->handle, UART_FLAG_PE))
	__HAL_UART_CLEAR_PEFLAG(hdl->handle);

	if (__HAL_UART_GET_FLAG(hdl->handle, UART_FLAG_FE))
	__HAL_UART_CLEAR_FEFLAG(hdl->handle);

	if (__HAL_UART_GET_FLAG(hdl->handle, UART_FLAG_NE))
	__HAL_UART_CLEAR_NEFLAG(hdl->handle);

	if (__HAL_UART_GET_FLAG(hdl->handle, UART_FLAG_ORE))
	__HAL_UART_CLEAR_OREFLAG(hdl->handle);

	//if (ISR & USART_ISR_TXE)
	if (((isrflags & USART_SR_TXE) != RESET) && ((cr1its & USART_CR1_TX_EN) != RESET))
	{
		if (hdl->tx_queue->in_index != hdl->tx_queue->out_index)
		{
#if USE_RS485>0
			if (hdl->dir_port != NULL)
			{
				HAL_GPIO_WritePin(hdl->dir_port, hdl->dir_pin, GPIO_PIN_SET); // transmitter on
				SET_BIT(hdl->handle->Instance->CR1, USART_CR1_TCIE); // Set transmission complete IRQ
			}
#endif
			c = hdl->tx_queue->buf[hdl->tx_queue->out_index & hdl->tx_queue->size_mask];
			__DMB();
			hdl->tx_queue->out_index++;

			hdl->handle->Instance->DR = c;
		}
		else
			CLEAR_BIT(hdl->handle->Instance->CR1, USART_CR1_TX_EN);
	}
}


#endif

#if defined STM32G0 || defined (STM32G4) || defined (STM32C0) || defined (STM32H5)|| defined (STM32U0) || defined (STM32U5)
/*!
 *
 * \brief -.
 *
 * \param hdl A handle to the SCI channel.
 *
 * \return -.
 */
static void isr_handler(const sci_hdl_t hdl)
{
	uint32_t isrflags = READ_REG(hdl->handle->Instance->ISR);
	uint32_t cr1its = READ_REG(hdl->handle->Instance->CR1);
	uint32_t cr3its = READ_REG(hdl->handle->Instance->CR3);
	uint32_t errorflags = 0x00U;

	sci_cb_args_t args;
	uint8_t c;

#if USE_RS485>0
	// clear the RS485 transmitter send
	if (hdl->dir_port != NULL)
	{
		if ((isrflags & USART_ISR_TC) != RESET)
		{
			HAL_GPIO_WritePin(hdl->dir_port, hdl->dir_pin, GPIO_PIN_RESET); // transmitter off
			CLEAR_BIT(hdl->handle->Instance->CR1, USART_CR1_TCIE); // Clear transmission complete
		}
	}
#endif

	/* If no error occurs */
	errorflags = (isrflags & (uint32_t) (USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));
	if (errorflags == RESET)
	{
		if (((isrflags & USART_ISR_RXNE_RXFNE) != 0U) && (((cr1its & USART_CR1_RXNEIE_RXFNEIE) != 0U) || ((cr3its & USART_CR3_RXFTIE) != 0U)))
		{
			c = hdl->handle->Instance->RDR;

			if ((hdl->rx_queue->in_index - hdl->rx_queue->out_index) <= hdl->rx_queue->size_mask)
			{
				hdl->rx_queue->buf[hdl->rx_queue->in_index & hdl->rx_queue->size_mask] = c;
				__DMB();
				hdl->rx_queue->in_index++;
			}

			// handle the callback
			if (hdl->callback != NULL)
			{
				args.event = SCI_EVT_RX_CHAR;
				args.hdl = hdl;
				args.byte = c;
				hdl->callback((void*) &args);
			}
		}
		else
			args.event = SCI_EVT_RXBUF_OVFL;
	}

	/* If some errors occur */
	if ((errorflags != 0U) && ((((cr3its & (USART_CR3_RXFTIE | USART_CR3_EIE)) != 0U) || ((cr1its & (USART_CR1_RXNEIE_RXFNEIE | USART_CR1_PEIE | USART_CR1_RTOIE)) != 0U))))
	{
		/* UART parity error interrupt occurred -------------------------------------*/
		if (((isrflags & USART_ISR_PE) != 0U) && ((cr1its & USART_CR1_PEIE) != 0U))
			__HAL_UART_CLEAR_FLAG(hdl->handle, UART_CLEAR_PEF);

		/* UART frame error interrupt occurred --------------------------------------*/
		if (((isrflags & USART_ISR_FE) != 0U) && ((cr3its & USART_CR3_EIE) != 0U))
			__HAL_UART_CLEAR_FLAG(hdl->handle, UART_CLEAR_FEF);

		/* UART noise error interrupt occurred --------------------------------------*/
		if (((isrflags & USART_ISR_NE) != 0U) && ((cr3its & USART_CR3_EIE) != 0U))
			__HAL_UART_CLEAR_FLAG(hdl->handle, UART_CLEAR_NEF);

		/* UART Over-Run interrupt occurred -----------------------------------------*/
		if (((isrflags & USART_ISR_ORE) != 0U) && (((cr1its & USART_CR1_RXNEIE_RXFNEIE) != 0U) || ((cr3its & (USART_CR3_RXFTIE | USART_CR3_EIE)) != 0U)))
			__HAL_UART_CLEAR_FLAG(hdl->handle, UART_CLEAR_OREF);

		/* UART Receiver Timeout interrupt occurred ---------------------------------*/
		if (((isrflags & USART_ISR_RTOF) != 0U) && ((cr1its & USART_CR1_RTOIE) != 0U))
			__HAL_UART_CLEAR_FLAG(hdl->handle, UART_CLEAR_RTOF);
	}

	if (((isrflags & USART_ISR_TXE_TXFNF) != 0U) && (((cr1its & USART_CR1_TXEIE_TXFNFIE) != 0U) || ((cr3its & USART_CR3_TXFTIE) != 0U)))
	{
		if (hdl->tx_queue->in_index != hdl->tx_queue->out_index)
		{
#if USE_RS485>0
			if (hdl->dir_port != NULL)
			{
				HAL_GPIO_WritePin(hdl->dir_port, hdl->dir_pin, GPIO_PIN_SET); // transmitter on
				SET_BIT(hdl->handle->Instance->CR1, USART_CR1_TCIE); // Set transmission complete IRQ
			}
#endif
			c = hdl->tx_queue->buf[hdl->tx_queue->out_index & hdl->tx_queue->size_mask];
			__DMB();
			hdl->tx_queue->out_index++;

			hdl->handle->Instance->TDR = c;
		}
		else
			CLEAR_BIT(hdl->handle->Instance->CR1, USART_CR1_TXEIE_TXFNFIE);
	}
}

#endif

/*!
 * \brief This function opens the SCI port.
 *
 * \param p_callback Pointer to function called from interrupt.
 *
 * \return True if successful, false on error.
 */
static bool sci_init(sci_ch_t ch, void (*const p_callback)(void *p_args))
{
	if (ch >= SCI_NUM_CH)
		return (false);

	g_handles[ch]->callback = p_callback;

	switch (ch)
	{
#if SCI_CFG_CH1_INCLUDED
	case SCI_CH1:
		g_handles[ch]->handle->Instance = USART1;
		g_handles[ch]->tx_queue->buf = ch1_tx_buf;
		g_handles[ch]->tx_queue->size_mask = (1 << SCI_CFG_CH1_TX_BUFSIZ_ORDER) - 1;
		g_handles[ch]->rx_queue->buf = ch1_rx_buf;
		g_handles[ch]->rx_queue->size_mask = (1 << SCI_CFG_CH1_RX_BUFSIZ_ORDER) - 1;
		__HAL_UART_ENABLE_IT(g_handles[ch]->handle, UART_IT_RXNE);
#if USE_RS485>0
		g_handles[ch]->dir_port = USART1_DIR_PORT;
		g_handles[ch]->dir_pin  = USART1_DIR_PIN;
#endif
		break;
#endif

#if SCI_CFG_CH2_INCLUDED
	case SCI_CH2:
		g_handles[ch]->handle->Instance = USART2;
		g_handles[ch]->tx_queue->buf = ch2_tx_buf;
		g_handles[ch]->tx_queue->size_mask = (1 << SCI_CFG_CH2_TX_BUFSIZ_ORDER) - 1;
		g_handles[ch]->rx_queue->buf = ch2_rx_buf;
		g_handles[ch]->rx_queue->size_mask = (1 << SCI_CFG_CH2_RX_BUFSIZ_ORDER) - 1;
		__HAL_UART_ENABLE_IT(g_handles[ch]->handle, UART_IT_RXNE);
#if USE_RS485>0
		g_handles[ch]->dir_port = USART2_DIR_PORT;
		g_handles[ch]->dir_pin  = USART2_DIR_PIN;
#endif
		break;
#endif

#if SCI_CFG_CH3_INCLUDED
	case SCI_CH3:
		g_handles[ch]->handle->Instance = USART3;
		g_handles[ch]->tx_queue->buf = ch3_tx_buf;
		g_handles[ch]->tx_queue->size_mask = (1 << SCI_CFG_CH3_TX_BUFSIZ_ORDER) - 1;
		g_handles[ch]->rx_queue->buf = ch3_rx_buf;
		g_handles[ch]->rx_queue->size_mask = (1 << SCI_CFG_CH3_RX_BUFSIZ_ORDER) - 1;
#if USE_RS485>0
		g_handles[ch]->dir_port = USART3_DIR_PORT;
		g_handles[ch]->dir_pin  = USART3_DIR_PIN;
#endif
		break;
#endif

#if SCI_CFG_CH4_INCLUDED
		case SCI_CH4:
		g_handles[ch]->handle->Instance = USART4;
		g_handles[ch]->tx_queue->buf = ch4_tx_buf;
		g_handles[ch]->tx_queue->size_mask = (1 << SCI_CFG_CH4_TX_BUFSIZ_ORDER) - 1;
		g_handles[ch]->rx_queue->buf = ch4_rx_buf;
		g_handles[ch]->rx_queue->size_mask = (1 << SCI_CFG_CH4_RX_BUFSIZ_ORDER) - 1;
#if USE_RS485>0
		g_handles[ch]->dir_port = USART4_DIR_PORT;
		g_handles[ch]->dir_pin  = USART4_DIR_PIN;
#endif
		break;
#endif

#if SCI_CFG_CH5_INCLUDED
		case SCI_CH5:
		g_handles[ch]->handle->Instance = UART5;
		g_handles[ch]->tx_queue->buf = ch5_tx_buf;
		g_handles[ch]->tx_queue->size_mask = (1 << SCI_CFG_CH5_TX_BUFSIZ_ORDER) - 1;
		g_handles[ch]->rx_queue->buf = ch5_rx_buf;
		g_handles[ch]->rx_queue->size_mask = (1 << SCI_CFG_CH5_RX_BUFSIZ_ORDER) - 1;
#if USE_RS485>0
		g_handles[ch]->dir_port = USART5_DIR_PORT;
		g_handles[ch]->dir_pin  = USART5_DIR_PIN;
#endif
		break;
#endif
#if SCI_CFG_CH6_INCLUDED
		case SCI_CH6:
		g_handles[ch]->handle->Instance = USART6;
		g_handles[ch]->tx_queue->buf = ch6_tx_buf;
		g_handles[ch]->tx_queue->size_mask = (1 << SCI_CFG_CH6_TX_BUFSIZ_ORDER) - 1;
		g_handles[ch]->rx_queue->buf = ch6_rx_buf;
		g_handles[ch]->rx_queue->size_mask = (1 << SCI_CFG_CH6_RX_BUFSIZ_ORDER) - 1;
#if USE_RS485>0
		g_handles[ch]->dir_port = USART6_DIR_PORT;
		g_handles[ch]->dir_pin  = USART6_DIR_PIN;
#endif
		break;
#endif

#if SCI_CFG_CH7_INCLUDED
		case SCI_CH7:
		g_handles[ch]->handle->Instance = USART7;
		g_handles[ch]->tx_queue->buf = ch7_tx_buf;
		g_handles[ch]->tx_queue->size_mask = (1 << SCI_CFG_CH7_TX_BUFSIZ_ORDER) - 1;
		g_handles[ch]->rx_queue->buf = ch7_rx_buf;
		g_handles[ch]->rx_queue->size_mask = (1 << SCI_CFG_CH7_RX_BUFSIZ_ORDER) - 1;
#if USE_RS485>0
		g_handles[ch]->dir_port = USART7_DIR_PORT;
		g_handles[ch]->dir_pin  = USART7_DIR_PIN;
#endif
		break;
#endif

#if SCI_CFG_CH8_INCLUDED
		case SCI_CH8:
		g_handles[ch]->handle->Instance = UART8;
		g_handles[ch]->tx_queue->buf = ch8_tx_buf;
		g_handles[ch]->tx_queue->size_mask = (1 << SCI_CFG_CH8_TX_BUFSIZ_ORDER) - 1;
		g_handles[ch]->rx_queue->buf = ch8_rx_buf;
		g_handles[ch]->rx_queue->size_mask = (1 << SCI_CFG_CH8_RX_BUFSIZ_ORDER) - 1;
#if USE_RS485>0
		g_handles[ch]->dir_port = USART8_DIR_PORT;
		g_handles[ch]->dir_pin  = USART8_DIR_PIN;
#endif
		break;
#endif

#if SCI_CFG_CH9_INCLUDED
		case SCI_CH9:
		g_handles[ch]->handle->Instance = USART9;
		g_handles[ch]->tx_queue->buf = ch9_tx_buf;
		g_handles[ch]->tx_queue->size_mask = (1 << SCI_CFG_CH9_TX_BUFSIZ_ORDER) - 1;
		g_handles[ch]->rx_queue->buf = ch8_rx_buf;
		g_handles[ch]->rx_queue->size_mask = (1 << SCI_CFG_CH9_RX_BUFSIZ_ORDER) - 1;
#if USE_RS485>0
		g_handles[ch]->dir_port = USART9_DIR_PORT;
		g_handles[ch]->dir_pin  = USART9_DIR_PIN;
#endif
		break;
#endif

#if SCI_CFG_LPCH1_INCLUDED
	case SCI_LPCH1:
		g_handles[ch]->handle->Instance = LPUART1;
		g_handles[ch]->tx_queue->buf = lpch1_tx_buf;
		g_handles[ch]->tx_queue->size_mask = (1 << SCI_CFG_LPCH1_TX_BUFSIZ_ORDER) - 1;
		g_handles[ch]->rx_queue->buf = lpch1_rx_buf;
		g_handles[ch]->rx_queue->size_mask = (1 << SCI_CFG_LPCH1_RX_BUFSIZ_ORDER) - 1;
#if USE_RS485>0
		g_handles[ch]->dir_port = USART9_DIR_PORT;
		g_handles[ch]->dir_pin  = USART9_DIR_PIN;
#endif
		break;
#endif

#if SCI_CFG_CAN_INCLUDED
	case SCI_CAN:
		g_handles[ch]->handle->Instance = NULL;
		g_handles[ch]->tx_queue->buf = can_tx_buf;
		g_handles[ch]->tx_queue->size_mask = (1 << SCI_CFG_CAN_TX_BUFSIZ_ORDER) - 1;
		g_handles[ch]->rx_queue->buf = can_rx_buf;
		g_handles[ch]->rx_queue->size_mask = (1 << SCI_CFG_CAN_RX_BUFSIZ_ORDER) - 1;
#if USE_RS485>0
		g_handles[ch]->dir_port = NULL;
		g_handles[ch]->dir_pin  = NULL;
#endif
		break;
#endif

#if SCI_CFG_USB_INCLUDED
	case SCI_USB:
	//	g_handles[ch]->handle->Instance = NULL;
		g_handles[ch]->tx_queue->buf = usb_tx_buf;
		g_handles[ch]->tx_queue->size_mask = (1 << SCI_CFG_USB_TX_BUFSIZ_ORDER) - 1;
		g_handles[ch]->rx_queue->buf = usb_rx_buf;
		g_handles[ch]->rx_queue->size_mask = (1 << SCI_CFG_USB_RX_BUFSIZ_ORDER) - 1;
#if USE_RS485>0
		g_handles[ch]->dir_port = NULL;
		g_handles[ch]->dir_pin  = NULL;
#endif
		break;
#endif

	default:
		break;
	}

	g_handles[ch]->tx_queue->in_index = 0;
	g_handles[ch]->tx_queue->out_index = 0;
	g_handles[ch]->rx_queue->in_index = 0;
	g_handles[ch]->rx_queue->out_index = 0;

	return (true);
}

/*!
 * \brief This function closes the SCI port.
 *
 * \param -.
 *
 * \return True if successful, false on error.
 */
static bool sci_close(sci_ch_t ch)
{
	if (ch >= SCI_NUM_CH)
		return false;
#if (SCI_CFG_CH1_INCLUDED || SCI_CFG_CH2_INCLUDED || SCI_CFG_CH3_INCLUDED || SCI_CFG_CH4_INCLUDED || SCI_CFG_CH5_INCLUDED || SCI_CFG_CH6_INCLUDED || SCI_CFG_CH7_INCLUDED || SCI_CFG_CH8_INCLUDED)
	if (HAL_UART_DeInit(g_handles[ch]->handle) != HAL_OK)
		return false;
#endif
	return true;
}

/*!
 * \brief This function writes a character to the SCI port.
 *
 * \param c Character to write.
 *
 * \return True if successful, false on error.
 */
static bool sci_putc(sci_ch_t ch, uint8_t c)
{
#if SCI_CFG_USB_INCLUDED
#if (SCI_USB_DEVICE)
	extern USBD_HandleTypeDef hUsbDeviceFS;
#else
	extern ApplicationTypeDef Appli_state;
#endif
#endif
	sci_hdl_t hdl = g_handles[ch];

	if (ch >= SCI_NUM_CH)
		return (false);

#if SCI_CFG_CAN_INCLUDED
	if (ch == SCI_CAN)
	{
		if ((hdl->tx_queue->in_index - hdl->tx_queue->out_index) <= hdl->tx_queue->size_mask)
		{
			hdl->tx_queue->buf[hdl->tx_queue->in_index & hdl->tx_queue->size_mask] = c;
			hdl->tx_queue->in_index++;
			return (true);
		}
		else
			return (false);
	}
	else
#endif
#if SCI_CFG_USB_INCLUDED
	if (ch == SCI_USB)
	{
		__DSB();
		__ISB();
		if ((hdl->tx_queue->in_index - hdl->tx_queue->out_index) <= hdl->tx_queue->size_mask)
		{
#if (SCI_USB_DEVICE)
			if (usb_tx_idle && (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED))
#else
			if (usb_tx_idle && (Appli_state == APPLICATION_READY))
#endif
			{
				usb_tx_idle = false;
#if (SCI_USB_DEVICE)
				CDC_Transmit_FS(&c, 1);
#else
				static uint8_t buff[2];
				buff[0] = c;
				USB_HOST_Transmit(buff, 1);
#endif
			}
			else
			{
				hdl->tx_queue->buf[hdl->tx_queue->in_index & hdl->tx_queue->size_mask] = c;
				__DMB();
				hdl->tx_queue->in_index++;
			}
			return (true);
		}
		else
			return (false);
	}
	else
#endif
	{
#if (SCI_CFG_CH1_INCLUDED || SCI_CFG_CH2_INCLUDED || SCI_CFG_CH3_INCLUDED || SCI_CFG_CH4_INCLUDED || SCI_CFG_CH5_INCLUDED || SCI_CFG_CH6_INCLUDED || SCI_CFG_CH7_INCLUDED || SCI_CFG_CH8_INCLUDED)

		CLEAR_BIT(hdl->handle->Instance->CR1, USART_CR1_TX_EN);
		__DSB();
		__ISB();

		if ((hdl->tx_queue->in_index - hdl->tx_queue->out_index) <= hdl->tx_queue->size_mask)
		{
			hdl->tx_queue->buf[hdl->tx_queue->in_index & hdl->tx_queue->size_mask] = c;
			__DMB();
			hdl->tx_queue->in_index++;

			SET_BIT(hdl->handle->Instance->CR1, USART_CR1_TX_EN);
			return (true);
		}
		else
			SET_BIT(hdl->handle->Instance->CR1, USART_CR1_TX_EN);
		return (false);
#endif
	}
}

/*!
 * \brief This function writes a zero terminated character string to the SCI port.
 *
 * \param str Pointer to the character string to write.
 *
 * \return True if successful, false on error.
 */
static bool sci_puts(sci_ch_t ch, uint8_t *str)
{
	bool err = true;

	while (*str && err)
		err = sci_putc(ch, *str++);

	return (err);
}

/*!
 * \brief This function writes a character string with a specified length to the SCI port.
 *
 * \param str Pointer to the character string to write.
 * \param len Length of the character string to write.
 *
 * \return True if successful, false on error.
 */
static bool sci_putsn(sci_ch_t ch, uint8_t *str, uint16_t len)
{
	bool err = true;

	while (len-- && err)
		err = sci_putc(ch, *str++);

	return (err);
}


/*!
 * \brief This function reads a character from the SCI port.
 *
 * \param c Pointer to the character read.
 *
 * \return True if successful, false on error.
 */
static bool sci_getc(sci_ch_t ch, uint8_t *c)
{
	sci_hdl_t hdl = g_handles[ch];

	if (ch >= SCI_NUM_CH)
		return (false);

#if SCI_CFG_CAN_INCLUDED || SCI_CFG_USB_INCLUDED
	if (ch == SCI_CAN || ch == SCI_USB)
	{
		if (hdl->rx_queue->in_index != hdl->rx_queue->out_index)
		{
			*c = hdl->rx_queue->buf[hdl->rx_queue->out_index & hdl->rx_queue->size_mask];
			__DMB();
			hdl->rx_queue->out_index++;
			return (true);
		}

		return (false);
	}
	else
#endif
	{
#if (SCI_CFG_CH1_INCLUDED || SCI_CFG_CH2_INCLUDED || SCI_CFG_CH3_INCLUDED || SCI_CFG_CH4_INCLUDED || SCI_CFG_CH5_INCLUDED || SCI_CFG_CH6_INCLUDED || SCI_CFG_CH7_INCLUDED || SCI_CFG_CH8_INCLUDED)
		CLEAR_BIT(hdl->handle->Instance->CR1, USART_CR1_RX_EN);
		__DSB();
		__ISB();

		if (hdl->rx_queue->in_index != hdl->rx_queue->out_index)
		{
			*c = hdl->rx_queue->buf[hdl->rx_queue->out_index & hdl->rx_queue->size_mask];
			__DMB();
			hdl->rx_queue->out_index++;

			SET_BIT(hdl->handle->Instance->CR1, USART_CR1_RX_EN);
			return (true);
		}
		else
			SET_BIT(hdl->handle->Instance->CR1, USART_CR1_RX_EN);
#endif
	}
	return (false);
}

/*!
 * \brief This function gets if bytes are waiting in the rx queue.
 *
 * \param
 *
 * \return True if bytes are in the queue
 */
static bool rx_queue_busy(sci_ch_t ch)
{
	sci_hdl_t hdl = g_handles[ch];

	if (ch >= SCI_NUM_CH)
		return (false);

	return (hdl->rx_queue->in_index != hdl->rx_queue->out_index);
}

/*!
 * \brief This function gets if bytes are waiting in the tx queue.
 *
 * \param
 *
 * \return True if bytes are in the queue
 */
static bool tx_queue_busy(sci_ch_t ch)
{
	sci_hdl_t hdl = g_handles[ch];

	if (ch >= SCI_NUM_CH)
		return (false);

	return (hdl->tx_queue->in_index != hdl->tx_queue->out_index);
}

#if 0
/*!
 * \brief This function gets the number of bytes waiting in the rx queue.
 *
 * \param cnt Pointer to the number of bytes waiting in the rx queue.
 *
 * \return True if successful, false on error.
 */
static bool sci_in_waiting(sci_ch_t ch, size_t *cnt)
{
	sci_hdl_t hdl = g_handles[ch];

	CLEAR_BIT(hdl->handle->Instance->CR1, USART_CR1_RX_EN);
	__DSB();
	__ISB();
	*cnt = hdl->rx_queue->count;
	SET_BIT(hdl->handle->Instance->CR1, USART_CR1_RX_EN);

	return (true);
}

/*!
 * \brief This function gets the number of bytes waiting in the tx queue.
 *
 * \param cnt Pointer to the number of bytes waiting in the tx queue.
 *
 * \return True if successful, false on error.
 */
static bool sci_out_waiting(sci_ch_t ch, size_t *cnt)
{
	sci_hdl_t hdl = g_handles[ch];

	CLEAR_BIT(hdl->handle->Instance->CR1, USART_CR1_TX_EN);
	__DSB();
	__ISB();
	*cnt = hdl->tx_queue->count;
	SET_BIT(hdl->handle->Instance->CR1, USART_CR1_TX_EN);

	return (true);
}
#endif

#if defined (STM32L4) || defined (STM32F0)|| defined (STM32L1) || defined (STM32U0)
static void sci_enable_stop_mode(sci_ch_t ch)
{
	sci_hdl_t hdl = g_handles[ch];
	UART_WakeUpTypeDef UART_WakeUpStruct =
	{ 0 };

	while (__HAL_UART_GET_FLAG(hdl->handle, USART_ISR_BUSY) == SET)
		;
	while (__HAL_UART_GET_FLAG(hdl->handle, USART_ISR_REACK) == RESET)
		;

	/* Set the wake-up event. */
	UART_WakeUpStruct.WakeUpEvent = UART_WAKEUP_ON_READDATA_NONEMPTY;
	if (HAL_UARTEx_StopModeWakeUpSourceConfig(hdl->handle, UART_WakeUpStruct) != HAL_OK)
	{
		while (1)
		{
			// Error.
		}
	}

	/* Enable the UART wake-up from stop mode Interrupt. */
	__HAL_UART_ENABLE_IT(hdl->handle, UART_IT_WUF);

	/* Enable MCU wake-up by UART. */
	HAL_UARTEx_EnableStopMode(hdl->handle);
}


static void sci_disable_stop_mode(sci_ch_t ch)
{
#if (SCI_CFG_CH1_INCLUDED || SCI_CFG_CH2_INCLUDED || SCI_CFG_CH3_INCLUDED || SCI_CFG_CH4_INCLUDED || SCI_CFG_CH5_INCLUDED || SCI_CFG_CH6_INCLUDED || SCI_CFG_CH7_INCLUDED || SCI_CFG_CH8_INCLUDED)

	sci_hdl_t hdl = g_handles[ch];

	/* Disable MCU wake-up by UART. */
	HAL_UARTEx_DisableStopMode(hdl->handle);
#endif
}
#endif

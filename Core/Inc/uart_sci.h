/*
 * uart_sci.h
 *
 *  Created on: 11 dec. 2020
 *      Author: Mark
 */

#ifndef UART_SCI_H_
#define UART_SCI_H_

#define BUFFER_256					(8)		// 2^8 = 256
#define BUFFER_512					(9)		// 2^9 = 512
#define BUFFER_1024					(10)	// 2^10 = 1024
#define BUFFER_2048					(11)	// 2^11 = 2048
#define BUFFER_4096					(12)	// 2^12 = 4096

#define SCI_CFG_CH1_INCLUDED       (1)
#define SCI_CFG_CH2_INCLUDED       (0)
#define SCI_CFG_CH3_INCLUDED       (0)
#define SCI_CFG_CH4_INCLUDED       (0)
#define SCI_CFG_CH5_INCLUDED       (0)
#define SCI_CFG_CH6_INCLUDED       (0)
#define SCI_CFG_CH7_INCLUDED       (0)
#define SCI_CFG_CH8_INCLUDED       (0)
#define SCI_CFG_CH9_INCLUDED       (0)
#define SCI_CFG_LPCH1_INCLUDED     (0)
#define SCI_CFG_CAN_INCLUDED	   (0)
#define SCI_CFG_USB_INCLUDED	   (0)

#if (SCI_CFG_USB_INCLUDED)
#define SCI_USB_DEVICE	(0)// Connect me to a PC
#if SCI_USB_DEVICE==0
#define SCI_USB_HOST	(1)// I can connect to a VPC Virtual Port Com
#endif
#endif

// In 2^ORDER
#define SCI_CFG_CH1_RX_BUFSIZ_ORDER   	(BUFFER_256)
#define SCI_CFG_CH1_TX_BUFSIZ_ORDER  	(BUFFER_1024)
#define SCI_CFG_CH2_RX_BUFSIZ_ORDER  	(BUFFER_2048)
#define SCI_CFG_CH2_TX_BUFSIZ_ORDER  	(BUFFER_1024)
#define SCI_CFG_CH3_RX_BUFSIZ_ORDER  	(BUFFER_256)
#define SCI_CFG_CH3_TX_BUFSIZ_ORDER		(BUFFER_256)
#define SCI_CFG_CH4_RX_BUFSIZ_ORDER    	(BUFFER_256)
#define SCI_CFG_CH4_TX_BUFSIZ_ORDER    	(BUFFER_256)
#define SCI_CFG_CH5_RX_BUFSIZ_ORDER    	(BUFFER_256)
#define SCI_CFG_CH5_TX_BUFSIZ_ORDER    	(BUFFER_256)
#define SCI_CFG_CH6_RX_BUFSIZ_ORDER    	(BUFFER_256)
#define SCI_CFG_CH6_TX_BUFSIZ_ORDER    	(BUFFER_256)
#define SCI_CFG_CH7_RX_BUFSIZ_ORDER    	(BUFFER_256)
#define SCI_CFG_CH7_TX_BUFSIZ_ORDER    	(BUFFER_256)
#define SCI_CFG_CH8_RX_BUFSIZ_ORDER    	(BUFFER_256)
#define SCI_CFG_CH8_TX_BUFSIZ_ORDER    	(BUFFER_256)
#define SCI_CFG_CH9_RX_BUFSIZ_ORDER    	(BUFFER_256)
#define SCI_CFG_CH9_TX_BUFSIZ_ORDER   	(BUFFER_256)
#define SCI_CFG_LPCH1_RX_BUFSIZ_ORDER   (BUFFER_256)
#define SCI_CFG_LPCH1_TX_BUFSIZ_ORDER   (BUFFER_1024)
#define SCI_CFG_CAN_RX_BUFSIZ_ORDER		(BUFFER_256)
#define SCI_CFG_CAN_TX_BUFSIZ_ORDER		(BUFFER_1024)
#define SCI_CFG_USB_RX_BUFSIZ_ORDER	    (BUFFER_2048)
#define SCI_CFG_USB_TX_BUFSIZ_ORDER 	(BUFFER_1024)

#define	FLEXIBLE_SCI1				(0) // direct tty_xx to flexible port or fix it below
#define	FLEXIBLE_SCI2				(0) // direct tty_xx to flexible port or fix it below
#define	FLEXIBLE_SCI3				(0) // direct tty_xx to flexible port or fix it below
#define	FLEXIBLE_SCI4				(0) // direct tty_xx to flexible port or fix it below
#define	FLEXIBLE_SCI5				(0) // direct tty_xx to flexible port or fix it below
#define	FLEXIBLE_SCI6				(0) // direct tty_xx to flexible port or fix it below
#define	FLEXIBLE_SCI7				(0) // direct tty_xx to flexible port or fix it below
#define	FLEXIBLE_SCI8				(0) // direct tty_xx to flexible port or fix it below
#define	FLEXIBLE_SCI9				(0) // direct tty_xx to flexible port or fix it below
#define	FLEXIBLE_SCI_LPCH1			(0) // direct tty_xx to flexible port or fix it below

#define	FLEXIBLE_CAN				(0) // direct tty_xx to flexible port or fix it below

#define USE_RS485					(0)

#if USE_RS485>0
#define	USART1_DIR_PORT				NULL
#define	USART1_DIR_PIN				0
#define	USART2_DIR_PORT				DIR2_GPIO_Port
#define	USART2_DIR_PIN				DIR2_Pin
#define	USART3_DIR_PORT				NULL
#define	USART3_DIR_PIN				0
#define	USART4_DIR_PORT				NULL
#define	USART4_DIR_PIN				0
#define	USART5_DIR_PORT				NULL
#define	USART5_DIR_PIN				0
#define	USART6_DIR_PORT				NULL
#define	USART6_DIR_PIN				0
#define	USART7_DIR_PORT				NULL
#define	USART7_DIR_PIN				0
#define	USART8_DIR_PORT				NULL
#define	USART8_DIR_PIN				0
#define	USART9_DIR_PORT				NULL
#define	USART9_DIR_PIN				0
#endif

#define ESC							27
#define TAB							9

#if defined (STM32G0) || defined (STM32C0) || defined (STM32U0)
#define USART_CR1_TX_EN		USART_CR1_TXEIE_TXFNFIE		/*!< TXE and TX FIFO Not Full Interrupt Enable */
#define USART_CR1_RX_EN		USART_CR1_RXNEIE_RXFNEIE	/*!< RXNE and RX FIFO Not Empty Interrupt Enable */
#else
#define USART_CR1_TX_EN		USART_CR1_TXEIE		/*!<TXE Interrupt Enable                   */
#define USART_CR1_RX_EN		USART_CR1_RXNEIE	/*!<RXNE Interrupt Enable                  */
#endif

// macro for tty redirection
#if FLEXIBLE_SCI1 || FLEXIBLE_SCI2 || FLEXIBLE_SCI3 || FLEXIBLE_SCI4 || FLEXIBLE_SCI5 || FLEXIBLE_SCI6 || FLEXIBLE_SCI7 || FLEXIBLE_SCI8 ||FLEXIBLE_SCI8 || FLEXIBLE_CAN

typedef struct
{
	bool (*sci_printf)(const char *fmt, ...);
	bool (*sci_putc)(char c);
	bool (*sci_puts)(char *str);
	bool (*sci_putsn)(char *str, uint16_t len);
	bool (*sci_getch)(char *ch);
#if USE_RS485>0
	bool (*sci_rxq_busy)(void);
	bool (*sci_txq_busy)(void);
#endif
} fp_t;

extern fp_t fp;

#define tty_printf(...)	fp.sci_printf(__VA_ARGS__)
#define tty_putc		fp.sci_putc
#define tty_puts		fp.sci_puts
#define tty_putsn		fp.sci_putsn
#define tty_getc		fp.sci_getch
#if USE_RS485>0
#define tty_rxq_busy	fp.sci_rxq_busy
#define tty_txq_busy	fp.sci_txq_busy
#endif
#else
#define tty_printf	sci1_printf // sci2_printf
#define tty_putc	sci1_putc
#define tty_puts	sci1_puts
#define tty_putsn	sci1_putsn
#define tty_getc	sci1_getch
#endif


typedef struct st_sci_queue
{
  uint8_t *buf;
  size_t size_mask;
  size_t in_index;
  size_t out_index;

} sci_queue_t;


typedef struct st_sci_ch_ctrl
{
#if (SCI_CFG_CH1_INCLUDED || SCI_CFG_CH2_INCLUDED || SCI_CFG_CH3_INCLUDED || SCI_CFG_CH4_INCLUDED || SCI_CFG_CH5_INCLUDED || SCI_CFG_CH6_INCLUDED || SCI_CFG_CH7_INCLUDED || SCI_CFG_CH8_INCLUDED)
  UART_HandleTypeDef *handle;
#endif
  sci_queue_t *tx_queue;
  sci_queue_t *rx_queue;
  void (*callback)(void *p_args);
#if USE_RS485>0
  GPIO_TypeDef * dir_port;
  uint16_t dir_pin;
#endif
} sci_ch_ctrl_t;

typedef enum e_sci_ch
{
  SCI_CH1 = 0,
  SCI_CH2,
  SCI_CH3,
  SCI_CH4,
  SCI_CH5,
  SCI_CH6,
  SCI_CH7,
  SCI_CH8,
  SCI_CH9,
  SCI_LPCH1,
  SCI_CAN,
  SCI_USB,
  SCI_NUM_CH
} sci_ch_t;

typedef struct st_sci_ch_ctrl *sci_hdl_t;

typedef enum e_sci_cb_evt
{
  SCI_EVT_RX_CHAR,
  SCI_EVT_RXBUF_OVFL,
  SCI_EVT_FRAMING_ERR,
  SCI_EVT_PARITY_ERR,
  SCI_EVT_OVFL_ERR
} sci_cb_evt_t;

typedef struct st_sci_cb_args
{
  sci_hdl_t hdl;
  sci_cb_evt_t event;
  uint8_t byte;
} sci_cb_args_t;

/*
 *
 */
void init_sci(void); // general initialization

#if (SCI_CFG_CH1_INCLUDED)
void sci1_callback(void);
bool sci1_getch(char *c);
bool sci1_putsn(char *str, uint16_t len);
bool sci1_putc(char c);
bool sci1_puts(char *str);
bool sci1_printf(const char *format, ...);
#if USE_RS485>0
bool sci1_rxq_busy(void);
bool sci1_txq_busy(void);
#endif
#if FLEXIBLE_SCI1
void shell_use_sci1(void);
#endif
#endif

#if (SCI_CFG_CH2_INCLUDED)
void sci2_callback(void);
bool sci2_getch(char *c);
bool sci2_putsn(char *str, uint16_t len);
bool sci2_putc(char c);
bool sci2_puts(char *str);
bool sci2_printf(const char *format, ...);
#if USE_RS485>0
bool sci2_rxq_busy(void);
bool sci2_txq_busy(void);
#endif
#if FLEXIBLE_SCI2
void shell_use_sci2(void);
#endif
#endif

#if (SCI_CFG_CH3_INCLUDED)
void sci3_callback(void);
bool sci3_getch(char *c);
bool sci3_putsn(char *str, uint16_t len);
bool sci3_putc(char c);
bool sci3_puts(char *str);
bool sci3_printf(char *format, ...);
#if USE_RS485>0
bool sci3_rxq_busy(void);
bool sci3_txq_busy(void);
#endif
#if FLEXIBLE_SCI3
void shell_use_sci3(void);
#endif
#endif

#if (SCI_CFG_CH4_INCLUDED)
void sci4_callback(void);
bool sci4_getch(char *c);
bool sci4_putsn(char *str, uint16_t len);
bool sci4_putc(char c);
bool sci4_puts(char *str);
bool sci4_printf(char *format, ...);
#if USE_RS485>0
bool sci4_rxq_busy(void);
bool sci4_txq_busy(void);
#endif
#if FLEXIBLE_SCI4
void shell_use_sci4(void);
#endif
#endif

#if (SCI_CFG_CH5_INCLUDED)
void sci5_callback(void);
bool sci5_getch(char *c);
bool sci5_putsn(char *str, uint16_t len);
bool sci5_putc(char c);
bool sci5_puts(char *str);
bool sci5_printf(char *format, ...);
#if USE_RS485>0
bool sci5_rxq_busy(void);
bool sci5_txq_busy(void);
#endif
#if FLEXIBLE_SCI5
void shell_use_sci5(void);
#endif
#endif

#if (SCI_CFG_CH6_INCLUDED)
void sci6_callback(void);
bool sci6_getch(char *c);
bool sci6_putsn(char *str, uint16_t len);
bool sci6_putc(char c);
bool sci6_puts(char *str);
bool sci6_printf(char *format, ...);
#if USE_RS485>0
bool sci6_rxq_busy(void);
bool sci6_txq_busy(void);
#endif
#if FLEXIBLE_SCI6
void shell_use_sci6(void);
#endif
#endif

#if (SCI_CFG_CH7_INCLUDED)
void sci7_callback(void);
bool sci7_getch(char *c);
bool sci7_putsn(char *str, uint16_t len);
bool sci7putc(char c);
bool sci7puts(char *str);
bool sci7_printf(char *format, ...);
#if USE_RS485>0
bool sci7_rxq_busy(void);
bool sci7_txq_busy(void);
#endif
#if FLEXIBLE_SCI7
void shell_use_sci7(void);
#endif
#endif

#if (SCI_CFG_CH8_INCLUDED)
void sci8_callback(void);
bool sci8_getch(char *c);
bool sci8_putsn(char *str, uint16_t len);
bool sci8_putc(char c);
bool sci8_puts(char *str);
bool sci8_printf(char *format, ...);
#if USE_RS485>0
bool sci8_rxq_busy(void);
bool sci8_txq_busy(void);
#endif
#if FLEXIBLE_SCI8
void shell_use_sci8(void);
#endif
#endif

#if (SCI_CFG_CH9_INCLUDED)
void sci9_callback(void);
bool sci9_getch(char *c);
bool sci9_putsn(char *str, uint16_t len);
bool sci9_putc(char c);
bool sci9_puts(char *str);
bool sci9_printf(char *format, ...);
#if USE_RS485>0
bool sci9_rxq_busy(void);
bool sci9_txq_busy(void);
#endif
#if FLEXIBLE_SCI9
void shell_use_sci9(void);
#endif
#endif

#if (SCI_CFG_LPCH1_INCLUDED)
void sci_lpch1_callback(void);
bool sci_lpch1_getch(char *c);
bool sci_lpch1_putsn(char *str, uint16_t len);
bool sci_lpch1_putc(char c);
bool sci_lpch1_puts(char *str);
bool sci_lpch1_printf(char *format, ...);
#if FLEXIBLE_SCI_LPCH1
void shell_use_sci_lpch1(void);
#endif
#endif

#if (SCI_CFG_CAN_INCLUDED)
#define CAN_DATASIZE 8 // can packet
#define S_BUF_SIZE 256
void sci_can_callback(void);
bool sci_can_getch(char *c);
bool sci_can_keyboard(char c);
bool sci_can_putsn(char *str, uint16_t len);
bool sci_can_putc(char c);
bool sci_can_puts(char *str);
bool sci_can_printf(const char *format, ...);
#if USE_RS485>0
bool sci_can_rxq_busy(void);
bool sci_can_txq_busy(void);
#endif
#if FLEXIBLE_CAN
void shell_use_can(void);
#endif
#endif


void sci_usb_callback(void);
void sci_usb_rxdata(uint8_t *Buf, uint32_t len);
bool sci_usb_getch(char *c);
bool sci_usb_putsn(char *str, uint16_t len);
bool sci_usb_putc(char c);
bool sci_usb_puts(char *str);
bool sci_usb_printf(const char *format, ...);
#if FLEXIBLE_USB
void shell_use_usb(void);
#endif




#endif /* UART_SCI_H_ */

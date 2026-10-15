#ifndef _YMODEM_H_
#define _YMODEM_H_

#include "stdbool.h"
#include "uart_sci.h"
#include "shell.h"

#define DEBUG_YMODEM   (0)



#define YMODEM_SEND				  (0) // send data to pc
#define YMODEM_RECEIVE			  (1) // receive data from pc (bootloader)

#define PACKET_SEQNO_INDEX        (1)
#define PACKET_SEQNO_COMP_INDEX   (2)
#define PACKET_START_INDEX        (1)
#define PACKET_NUMBER_INDEX       (2)
#define PACKET_CNUMBER_INDEX      (3)
#define PACKET_DATA_INDEX         (4)
#define PACKET_HEADER_SIZE        (3)
#define PACKET_TRAILER_SIZE       (2)
#define PACKET_HEADER             (3)
#define PACKET_TRAILER            (2)
#define PACKET_OVERHEAD           (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE               (128)
#define PACKET_1K_SIZE            (1024)
#define TICK_PER_SEC              (1000)
#define PACKET_TIMEOUT            (TICK_PER_SEC)

#define FILE_NAME_LENGTH          (256)
#define FILE_SIZE_LENGTH          (16)

#define SOH                       (0x01) // start of 128-byte data packet
#define STX                       (0x02) // start of 1024-byte data packet
#define EOT                       (0x04) // end of transmission
#define ACK                       (0x06) // acknowledge
#define NAK                       (0x15) // negative acknowledge
#define CCAN                      (0x18) // two of these in succession aborts transfer
#define CRC16                     (0x43) // 'C' request 16-bit CRC
#define ABORT                     (0x1B) // 'ESC' abort by user
#define CA                        (0x18) // two of these in succession aborts transfer

#define MAX_ERRORS                (5)

#define IS_AF(c)                  ((c >= 'A') && (c <= 'F'))
#define IS_af(c)                  ((c >= 'a') && (c <= 'f'))
#define IS_09(c)                  ((c >= '0') && (c <= '9'))
#define ISVALIDHEX(c)             (IS_AF(c) || IS_af(c) || IS_09(c))
#define ISVALIDDEC(c)             (IS_09(c))
#define CONVERTDEC(c)             (c - '0')
#define CONVERTHEX_alpha(c)       ((IS_AF(c) ? (c - 'A' + 10) : (c - 'a' + 10)))
#define CONVERTHEX(c)             ((IS_09(c) ? (c - '0') : CONVERTHEX_alpha(c)))

// Wrappers
#define ym_putc(a)       (tty_putc(a))
#define ym_putsn(a, b)   (tty_putsn(a, b))
#define ym_getc(a)       (tty_getc(a))



#if DEBUG_YMODEM > 0
#define debug_msg(...)   (sci2_printf(__VA_ARGS__))
#else
#define debug_msg(...) NULL
#endif


typedef struct
{
  char *filename;
  uint32_t filesize;
  bool (*f_open)       (const char *filename);
  bool (*f_read)       (uint8_t *p_data, uint32_t len);
  bool (*f_close)      (void);
} ymt_data_t;


typedef enum
{
  COM_OK = 0,
  COM_ERROR,
  COM_ABORT,
  COM_TIMEOUT,
  COM_DATA,
  COM_LIMIT
} com_err_t;

typedef struct
{
  uint32_t addr;
  uint32_t size;
  uint8_t *p_buf;
  uint32_t len;
} ym_data_t;

// Public function prototypes
#if YMODEM_RECEIVE > 0
int32_t ymodem_receive(uint32_t address, uint32_t size, bool (*callback)(void *p_args));
#endif

#if YMODEM_SEND > 0
com_err_t ymodem_transmit(ymt_data_t ym_data);
// user defined functions for upload to pc
bool flash_open(const char *filename);
bool flash_read(uint8_t *p_data, uint32_t len);
bool flash_close(void);
#endif


#endif

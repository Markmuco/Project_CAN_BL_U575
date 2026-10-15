/*!
 * \file n25lpxx.h
 *
 * \brief This software module provides I/O functions for a n25lpxx.
 */
#ifndef _n25lpxx_H_
#define _n25lpxx_H_

#include <stdbool.h>
#include <stdint.h>

#define FLASH_2MB	(1) // IS25LP016A:	16Mbit/2Mbyte
#define FLASH_4MB	(0) // IS25LP032A:	32Mbit/4Mbyte
#define FLASH_8MB	(0) // IS25LP064A:	64Mbit/8Mbyte
#define FLASH_16MB	(0) // IS25LP128:	128M-bit/16M-byte Print:9D.60.18
#define FLASH_32MB	(0) // S25LP256D


// n25lpxx memory organization
#define MANUFACTURER_ID                  0x9D  // Device identification
#define MEMORY_TYPE_4MB                  0x40
#define MEMORY_TYPE_8MB                  0x60

#define CAPACITY_2MB                     0x15 // not verified
#define CAPACITY_4MB                     0x16
#define CAPACITY_8MB                     0x17
#define CAPACITY_16MB                    0x18

#define DEEP_SLEEP                       0xB9
#define READ_PRODUCT_IDENTIFICATION 	 0xAB

#if FLASH_2MB>0
#define N25LPXX_PAGE_COUNT            (8192) // NUMBER OF PAGES
#define N25LPXX_PAGE_SIZE              (256) // BYTES PER PAGE
#define N25LPXX_PAGE_OFFSET              (8) // OFFSET IN ADDRESS

#define N25LPXX_SECTOR_COUNT           (512) // NUMBER OF SUB-SECTORS
#define N25LPXX_SECTOR_SIZE           (4096) // BYTES PER SUB-SECTOR

#define N25LPXX_BLOCK32K_COUNT          (64) // NUMBER OF SECTORS
#define N25LPXX_BLOCK32K_SIZE        (32768) // BYTES PER SECTOR

#define N25LPXX_BLOCK64K_COUNT          (32) // NUMBER OF SECTORS
#define N25LPXX_BLOCK64K_SIZE        (65536) // BYTES PER SECTOR

#define N25LPXX_DIE_SIZE           (2097152) // BYTES PER DIE
#endif

#if FLASH_4MB>0
#define N25LPXX_PAGE_COUNT           (16384) // NUMBER OF PAGES
#define N25LPXX_PAGE_SIZE              (256) // BYTES PER PAGE
#define N25LPXX_PAGE_OFFSET              (8) // OFFSET IN ADDRESS

#define N25LPXX_SECTOR_COUNT          (1024) // NUMBER OF SUB-SECTORS
#define N25LPXX_SECTOR_SIZE           (4096) // BYTES PER SUB-SECTOR

#define N25LPXX_BLOCK32K_COUNT         (128) // NUMBER OF SECTORS
#define N25LPXX_BLOCK32K_SIZE        (32768) // BYTES PER SECTOR

#define N25LPXX_BLOCK64K_COUNT          (64) // NUMBER OF SECTORS
#define N25LPXX_BLOCK64K_SIZE        (65536) // BYTES PER SECTOR

#define N25LPXX_DIE_SIZE           (4194304) // BYTES PER DIE
#endif


#if FLASH_8MB>0
#define N25LPXX_PAGE_COUNT           (32768) // NUMBER OF PAGES
#define N25LPXX_PAGE_SIZE              (256) // BYTES PER PAGE
#define N25LPXX_PAGE_OFFSET              (8) // OFFSET IN ADDRESS

#define N25LPXX_SECTOR_COUNT       	(2048) // NUMBER OF SUB-SECTORS
#define N25LPXX_SECTOR_SIZE        	(4096) // BYTES PER SUB-SECTOR

#define N25LPXX_BLOCK32K_COUNT         (256) // NUMBER OF SECTORS
#define N25LPXX_BLOCK32K_SIZE        (32768) // BYTES PER SECTOR

#define N25LPXX_BLOCK64K_COUNT         (128) // NUMBER OF SECTORS
#define N25LPXX_BLOCK64K_SIZE        (65536) // BYTES PER SECTOR

#define N25LPXX_DIE_SIZE           (8388608) // BYTES PER DIE
#endif

#if FLASH_16MB>0
#define N25LPXX_PAGE_COUNT           (65536) // NUMBER OF PAGES
#define N25LPXX_PAGE_SIZE              (256) // BYTES PER PAGE
#define N25LPXX_PAGE_OFFSET              (8) // OFFSET IN ADDRESS

#define N25LPXX_SECTOR_COUNT       	(4096) // NUMBER OF SUB-SECTORS
#define N25LPXX_SECTOR_SIZE        	(4096) // BYTES PER SUB-SECTOR

#define N25LPXX_BLOCK32K_COUNT         (512) // NUMBER OF SECTORS
#define N25LPXX_BLOCK32K_SIZE        (32768) // BYTES PER SECTOR

#define N25LPXX_BLOCK64K_COUNT         (256) // NUMBER OF SECTORS
#define N25LPXX_BLOCK64K_SIZE        (65536) // BYTES PER SECTOR

#define N25LPXX_DIE_SIZE          (16776960) // BYTES PER DIE
#endif

#define BLOCK_SIZE					N25LPXX_SECTOR_SIZE

// Shadow application storage on the external N25LPxx: whole chip, starting at address 0.
#define N25LPXX_SHADOW_START_ADDR	(0x000000UL)
#define N25LPXX_SHADOW_SIZE			(N25LPXX_DIE_SIZE)

// n25lpxx erase commands
typedef enum e_n25lpxx_erase_cmd
{
//  n25lpxx_SUBSECTOR_ERASE_PAGE = XX, // no page erase
  N25LPXX_SECTOR_ERASE      = 0XD7, // 4K SECTOR ERASE OPERATION (SER, D7H/20H)
  N25LPXX_BLOCK32K_ERASE    = 0X52, // 32K
  N25LPXX_BLOCK64K_ERASE    = 0XD8, // 64K
  N25LPXX_DIE_ERASE         = 0XC7  // CHIP ERASE OPERATION (CER, C7H/60H)
} n25lpxx_erase_cmd_t;

/*!
 * \typedef fls_err_t
 *
 * \brief Enum of FLASH API error codes.
 */
typedef enum
{
  FLS_SUCCESS = 0,    //!< (0) No errors.
  FLS_ERR_FAILURE,    //!< (1) Identification error.
  FLS_ERR_BUSY,       //!< (2) Flash is busy writing / erasing.
  FLS_ERR_PARAM,      //!< (3) Illegal parameter.
  FLS_ERR_PAGE,       //!< (4) Invalid page number.
  FLS_ERR_OVERFLOW,   //!< (5) Address + number of bytes for this operation went past the end of this memory area.
  FLS_ERR_CHECKSUM,   //!< (6) Invalid checksum.
  FLS_ERR_COMPARE,    //!< (7) Flash compare error.
  FLS_ERR_EPE,        //!< (8) Erase Program error GVL
  FLS_HAL_ERR,        //!<

} fls_err_t;


// Public function prototypes
fls_err_t n25lpxx_init(void);
fls_err_t n25lpxx_read(uint8_t *p_dst, uint32_t address, uint32_t size);
fls_err_t n25lpxx_write(const uint8_t *p_src, uint32_t address, uint32_t size);
fls_err_t n25lpxx_erase(uint32_t address, uint32_t size);
fls_err_t n25lpxx_blank_check(uint32_t *address);
fls_err_t n25lpxx_deep_sleep(void);
fls_err_t n25lpxx_wake_sleep(void);

// Wrapper for YMODEM (matches the bool(*)(void *p_args) callback signature).
bool n25lpxx_ymodem_write(void *p_args);

#endif

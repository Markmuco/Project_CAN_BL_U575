/*!
 * \file utilities.h
 *
 * \brief This software module provides utility functions.
 */
#ifndef _UTILITIES_H_
#define _UTILITIES_H_

// Include / exclude functions
#define USE_MY_ATOI       (0)
#define USE_MY_ATOL       (0)
#define USE_MY_ITOA       (1)
#define USE_MY_UITOA      (0)
#define USE_MY_LTOA       (0)
#define USE_MY_ULTOA      (0)
#define USE_MY_STRTOL     (0)
#define USE_MY_STRTOUL    (0)

#define USE_MY_STRCPY     (0)
#define USE_MY_STRCAT     (0)

#define USE_STRTOUPPER    (1)
#define USE_STRNTOUPPER   (0)
#define USE_STRTOLOWER    (0)
#define USE_STRNTOLOWER   (0)

#define USE_SWAP_INT16    (0)
#define USE_SWAP_UINT16   (0)
#define USE_SWAP_INT32    (0)
#define USE_SWAP_UINT32   (0)
#define USE_SWAP_INT64    (0)
#define USE_SWAP_UINT64   (0)

#define USE_BIN2BCD       (0)
#define USE_BCD2BIN       (0)

#define USE_MAKEWORD      (0)
#define USE_MAKELONG      (0)

// Public Function prototypes
#if (USE_MY_ATOI > 0)
int my_atoi(const char *str);
#endif
#if (USE_MY_ATOL > 0)
long my_atol(const char *str);
#endif
#if (USE_MY_ITOA > 0)
char *my_itoa(int value, char *str, int base);
#endif
#if (USE_MY_UITOA > 0)
char *my_uitoa(unsigned int value, char *str, int base);
#endif
#if (USE_MY_LTOA > 0)
char *my_ltoa(long value, char *str, int base);
#endif
#if (USE_MY_ULTOA > 0)
char *my_ultoa(unsigned long value, char *str, int base);
#endif
#if (USE_MY_STRTOL > 0)
long my_strtol(const char *nptr, char **endptr, int base);
#endif
#if (USE_MY_STRTOUL > 0)
unsigned long my_strtoul(const char *nptr, char **endptr, int base);
#endif

#if (USE_MY_STRCPY > 0)
char *my_strcpy(char *p_dst, const char *p_src);
#endif
#if (USE_MY_STRCAT > 0)
char *my_strcat(char *p_dst, const char *p_src);
#endif

#if (USE_STRTOUPPER > 0)
char *strtoupper(char *s);
#endif
#if (USE_STRNTOUPPER > 0)
char *strntoupper(char *s, int n);
#endif
#if (USE_STRTOLOWER > 0)
char *strtolower(char *s);
#endif
#if (USE_STRNTOLOWER > 0)
char *strntolower(char *s, int n);
#endif

#if (USE_SWAP_INT16 > 0)
int16_t swap_int16(int16_t value);
#endif
#if (USE_SWAP_UINT16 > 0)
uint16_t swap_uint16(uint16_t value);
#endif
#if (USE_SWAP_INT32 > 0)
int32_t swap_int32(int32_t value);
#endif
#if (USE_SWAP_UINT32 > 0)
uint32_t swap_uint32(uint32_t value);
#endif
#if (USE_SWAP_INT64 > 0)
int64_t swap_int64(int64_t value);
#endif
#if (USE_SWAP_UINT64 > 0)
uint64_t swap_uint64(uint64_t value);
#endif

#if (USE_BIN2BCD > 0)
uint8_t bin2bcd(uint8_t value);
#endif
#if (USE_BCD2BIN > 0)
uint8_t bcd2bin(uint8_t value);
#endif

#if (USE_MAKEWORD > 0)
uint16_t makeword(uint8_t hb, uint8_t lb);
#endif

#if (USE_MAKELONG > 0)
uint32_t makelong(uint16_t hw, uint16_t lw);
#endif


void itos(char *s, int32_t value, uint8_t radix, uint8_t minlen);

#endif

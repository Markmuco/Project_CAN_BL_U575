/*!
 * \file utilities.c
 *
 * \brief This software module provides utility functions.
 */
#include <stdbool.h>
#include <stdint.h>
#include "utilities.h"
#include "string.h"
#include "ctype.h"
#if ((USE_MY_STRTOL > 0) || (USE_MY_STRTOUL > 0))

#define LONG_MIN    (-2147483647)
#define LONG_MAX    (2147483647)

#define ULONG_MIN   (0)
#define ULONG_MAX   (4294967295)

#endif

#if 0
static inline int isupper(char c)
{
  return ((c >= 'A') && (c <= 'Z'));
}

static inline int islower(char c)
{
  return ((c >= 'a') && (c <= 'z'));
}

static inline int isalpha(char c)
{
  return (((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')));
}

static inline int isspace(char c)
{
  return ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\12'));
  //return ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\v') || (c == '\f') || (c == '\r'));
}

static inline int isdigit(char c)
{
  return ((c >= '0') && (c <= '9'));
}

static inline char toupper(char c)
{
  if (islower(c))
    c -= 0x20;

  return (c);
}

static inline char tolower(char c)
{
  if (isupper(c))
    c += 0x20;

  return (c);
}
#endif
#if (USE_MY_ATOI > 0)
/*!
 * \brief This function converts a string to a integer.
 *
 * \param str A pointer to a string beginning with the representation of an integral number.
 *
 * \return On success, the function returns the converted integral number as an integer value.
 */
int my_atoi(const char *str)
{
  int n = 0;
  bool neg = false;

  while ((*str == ' ') || (*str == '\t'))
    str++;

  if (*str == '+')
    str++;
  else if (*str == '-')
  {
    str++;
    neg = TRUE;
  }

  while ((*str >= '0') && (*str <= '9'))
  {
    n = n * 10 + (*str - '0');
    str++;
  }

  return (neg ? -n : n);
}
#endif




#if (USE_MY_ATOL > 0)
/*!
 * \brief This function converts a string to a long integer.
 *
 * \param str A pointer to a string beginning with the representation of an integral number.
 *
 * \return On success, the function returns the converted integral number as a long integer value.
 */
long my_atol(const char *str)
{
  long n = 0;
  bool neg = false;

  while ((*str == ' ') || (*str == '\t'))
    str++;

  if (*str == '+')
    str++;
  else if (*str == '-')
  {
    str++;
    neg = true;
  }

  while ((*str >= '0') && (*str <= '9'))
  {
    n = n * 10 + (*str - '0');
    str++;
  }

  return (neg ? -n : n);
}
#endif

#if (USE_MY_ITOA > 0)
/*!
 * \brief This function converts an integer value to a null-terminated string.
 *
 * \param value The value to be converted to a string.
 * \param str An array in memory where to store the resulting null-terminated string.
 * \param base A numerical base used to represent the value as a string.
 *
 * \return A pointer to the resulting null-terminated string, same as parameter str.
 */
char *my_itoa(int value, char *str, int base)
{
  char *dst;
  char digits[32];
  unsigned int x;
  int i, n;

  dst = str;
  if ((base < 2) || (base > 36))
  {
    *dst = 0;
    return (str);
  }

  if ((base == 10) && (value < 0))
  {
    *dst++ = '-';
    x = -value;
  }
  else
    x = value;

  i = 0;
  do
  {
    n = x % (unsigned int)base;
    digits[i++] = ((n < 10) ? (char)n + '0' : (char)n - 10 + 'a');
    x /= (unsigned int)base;
  } while (x != 0);

  while (i > 0)
    *dst++ = digits[--i];
  *dst = 0;

  return (str);
}
#endif

#if (USE_MY_UITOA > 0)
/*!
 * \brief This function converts an unsigned integer value to a null-terminated string.
 *
 * \param value The value to be converted to a string.
 * \param str An array in memory where to store the resulting null-terminated string.
 * \param base A numerical base used to represent the value as a string.
 *
 * \return A pointer to the resulting null-terminated string, same as parameter str.
 */
char *my_uitoa(unsigned int value, char *str, int base)
{
  char *dst;
  char digits[32];
  int i, n;

  dst = str;
  if ((base < 2) || (base > 36))
  {
    *dst = 0;
    return (str);
  }

  i = 0;
  do
  {
    n = value % (unsigned int)base;
    digits[i++] = ((n < 10) ? (char)n + '0' : (char)n - 10 + 'a');
    value /= (unsigned int)base;
  } while (value != 0);

  while (i > 0)
    *dst++ = digits[--i];
  *dst = 0;

  return (str);
}
#endif

#if (USE_MY_LTOA > 0)
/*!
 * \brief This function converts an long integer value to a null-terminated string.
 *
 * \param value The value to be converted to a string.
 * \param str An array in memory where to store the resulting null-terminated string.
 * \param base A numerical base used to represent the value as a string.
 *
 * \return A pointer to the resulting null-terminated string, same as parameter str.
 */
char *my_ltoa(long value, char *str, int base)
{
  char *dst;
  char digits[32];
  unsigned long x;
  int i, n;

  dst = str;
  if ((base < 2) || (base > 36))
  {
    *dst = 0;
    return (str);
  }

  if ((base == 10) && (value < 0))
  {
    *dst++ = '-';
    x = -value;
  }
  else
    x = value;

  i = 0;
  do
  {
    n = x % (unsigned long)base;
    digits[i++] = ((n < 10) ? (char)n + '0' : (char)n - 10 + 'a');
    x /= (unsigned long)base;
  } while (x != 0);

  while (i > 0)
    *dst++ = digits[--i];
  *dst = 0;

  return (str);
}
#endif

#if (USE_MY_ULTOA > 0)
/*!
 * \brief This function converts an unsigned long integer value to a null-terminated string.
 *
 * \param value The value to be converted to a string.
 * \param str An array in memory where to store the resulting null-terminated string.
 * \param base A numerical base used to represent the value as a string.
 *
 * \return A pointer to the resulting null-terminated string, same as parameter str.
 */
char *my_ultoa(unsigned long value, char *str, int base)
{
  char *dst;
  char digits[32];
  int i, n;

  dst = str;
  if ((base < 2) || (base > 36))
  {
    *dst = 0;
    return (str);
  }

  i = 0;
  do
  {
    n = value % (unsigned long)base;
    digits[i++] = ((n < 10) ? (char)n + '0' : (char)n - 10 + 'a');
    value /= (unsigned long)base;
  } while (value != 0);

  while (i > 0)
    *dst++ = digits[--i];
  *dst = 0;

  return (str);
}
#endif

#if (USE_MY_STRTOL > 0)
/*!
 * \brief This function converts a string to a long integer.
 *
 * \param nptr A string beginning with the representation of an integral number.
 * \param endptr Reference to an object of type char *, whose value is set by the function to the next character in str after the numerical value.
 * \param base Numerical base (radix) that determines the valid characters and their interpretation.
 *
 * \return On success, the function returns the converted integral number as a long integer value.
 */
long my_strtol(const char *nptr, char **endptr, int base)
{
  const char *s = nptr;
  unsigned long acc;
  int c;
  unsigned long cutoff;
  int neg = 0, any, cutlim;

  /*
   * Skip white space and pick up leading +/- sign if any.
   * If base is 0, allow 0x for hex and 0 for octal, else
   * assume decimal; if base is already 16, allow 0x.
   */
  do
  {
    c = *s++;
  } while (isspace(c));

  if (c == '-')
  {
    neg = 1;
    c = *s++;
  }
  else if (c == '+')
    c = *s++;

  if (((base == 0) || (base == 16)) && (c == '0') && ((*s == 'x') || (*s == 'X')))
  {
    c = s[1];
    s += 2;
    base = 16;
  }
  else if (((base == 0) || (base == 2)) && (c == '0') && ((*s == 'b') || (*s == 'B')))
  {
    c = s[1];
    s += 2;
    base = 2;
  }

  if (base == 0)
    base = (c == '0' ? 8 : 10);

  /*
   * Compute the cutoff value between legal numbers and illegal
   * numbers.  That is the largest legal value, divided by the
   * base.  An input number that is greater than this value, if
   * followed by a legal input character, is too big.  One that
   * is equal to this value may be valid or not; the limit
   * between valid and invalid numbers is then based on the last
   * digit.  For instance, if the range for longs is
   * [-2147483648..2147483647] and the input base is 10,
   * cutoff will be set to 214748364 and cutlim to either
   * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
   * a value > 214748364, or equal but the next digit is > 7 (or 8),
   * the number is too big, and we will return a range error.
   *
   * Set any if any `digits' consumed; make it negative to indicate
   * overflow.
   */
  cutoff = (neg ? -(unsigned long)LONG_MIN : LONG_MAX);
  cutlim = cutoff % (unsigned long)base;
  cutoff /= (unsigned long)base;
  for (acc = 0, any = 0; ;c = *s++)
  {
    if (isdigit(c))
      c -= '0';
    else if (isalpha(c))
      c -= isupper(c) ? 'A' - 10 : 'a' - 10;
    else
      break;

    if (c >= base)
      break;

    if ((any < 0) || (acc > cutoff) || ((acc == cutoff) && (c > cutlim)))
      any = -1;
    else
    {
      any = 1;
      acc *= base;
      acc += c;
    }
  }

  if (any < 0)
    acc = (neg ? LONG_MIN : LONG_MAX);
  else if (neg)
    acc = -acc;

  if (endptr != 0)
    *endptr = (char *)(any ? s - 1 : nptr);

  return (acc);
}
#endif

#if (USE_MY_STRTOUL > 0)
/*!
 * \brief This function converts a string to an unsigned long integer.
 *
 * \param nptr A string beginning with the representation of an integral number.
 * \param endptr Reference to an object of type char *, whose value is set by the function to the next character in str after the numerical value.
 * \param base Numerical base (radix) that determines the valid characters and their interpretation.
 *
 * \return On success, the function returns the converted integral number as an unsigned long integer value.
 */
unsigned long my_strtoul(const char *nptr, char **endptr, int base)
{
  const char *s = nptr;
  unsigned long acc;
  int c;
  unsigned long cutoff;
  int neg = 0, any, cutlim;

  /*
   * See strtol for comments as to the logic used.
   */
  do
  {
    c = *s++;
  } while (isspace(c));

  if (c == '-')
  {
    neg = 1;
    c = *s++;
  }
  else if (c == '+')
    c = *s++;

  if (((base == 0) || (base == 16)) && (c == '0') && ((*s == 'x') || (*s == 'X')))
  {
    c = s[1];
    s += 2;
    base = 16;
  }
  else if (((base == 0) || (base == 2)) && (c == '0') && ((*s == 'b') || (*s == 'B')))
  {
    c = s[1];
    s += 2;
    base = 2;
  }
  if (base == 0)
    base = (c == '0' ? 8 : 10);

  cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
  cutlim = (unsigned long)ULONG_MAX % (unsigned long)base;
  for (acc = 0, any = 0; ;c = *s++)
  {
    if (isdigit(c))
      c -= '0';
    else if (isalpha(c))
      c -= isupper(c) ? 'A' - 10 : 'a' - 10;
    else
      break;

    if (c >= base)
      break;

    if ((any < 0) || (acc > cutoff) || ((acc == cutoff) && (c > cutlim)))
      any = -1;
    else
    {
      any = 1;
      acc *= base;
      acc += c;
    }
  }

  if (any < 0)
    acc = ULONG_MAX;
  else if (neg)
    acc = -acc;

  if (endptr != 0)
    *endptr = (char *)(any ? s - 1 : nptr);

  return (acc);
}
#endif

#if (USE_MY_STRCPY > 0)
/*!
 * \brief This function copies the string pointed by source into the array pointed by destination.
 *
 * \param p_dst A pointer to the destination array where the content is to be copied.
 * \param p_src The string to be copied.
 *
 * \return A pointer to the last character in the destination.
 */
char *my_strcpy(char *p_dst, const char *p_src)
{
  while (*p_src)
  {
    *p_dst++ = *p_src++;
  }
  *p_dst = '\0';

  return (p_dst);
}
#endif

#if (USE_MY_STRCAT > 0)
/*!
 * \brief This function appends a copy of the source string to the destination string.
 *
 * \param p_dst A pointer to the destination array where the content is to be copied.
 * \param p_src The string to be copied.
 *
 * \return A pointer to the last character in the destination.
 */
char *my_strcat(char *p_dst, const char *p_src)
{
  while (*p_src)
  {
    *p_dst++ = *p_src++;
  }
  *p_dst = '\0';

  return (p_dst);
}
#endif

#if (USE_STRTOUPPER > 0)
/*!
 * \brief This function converts the given string to upper case.
 *
 * \param s A pointer to the string to convert to upper case.
 *
 * \return A pointer to the string converted to upper case.
 */
char *strtoupper(char *s)
{
  char *p = s;

  while (*p != '\0')
  {
    *p = toupper(*p);
    p++;
  }

  return (s);
}
#endif

#if (USE_STRNTOUPPER > 0)
/*!
 * \brief This function converts the given string to upper case.
 *
 * \param s A pointer to the string to convert to upper case.
 * \param n The maximum number of characters to convert.
 *
 * \return A pointer to the string converted to upper case.
 */
char *strntoupper(char *s, int n)
{
  char *p = s;

  if (n == 0)
    return (s);

  while ((n > 0) && (*p != '\0'))
  {
    *p = toupper(*p);
    p++;
    n--;
  }

  return (s);
}
#endif

#if (USE_STRTOLOWER > 0)
/*!
 * \brief This function converts the given string to lower case.
 *
 * \param s A pointer to the string to convert to lower case.
 *
 * \return A pointer to the string converted to lower case.
 */
char *strtolower(char *s)
{
  char *p = s;

  while (*p != '\0')
  {
    *p = tolower(*p);
    p++;
  }

  return (s);
}
#endif

#if (USE_STRNTOLOWER > 0)
/*!
 * \brief This function converts the given string to lower case.
 *
 * \param s A pointer to the string to convert to lower case.
 * \param n The maximum number of characters to convert.
 *
 * \return A pointer to the string converted to lower case.
 */
char *strntolower(char *s, int n)
{
  char *p = s;

  if (n == 0)
    return (s);

  while ((n > 0) && (*p != '\0'))
  {
    *p = tolower(*p);
    p++;
    n--;
  }

  return (s);
}
#endif

#if (USE_SWAP_INT16 > 0)
int16_t swap_int16(int16_t value)
{
  return (value << 8) | ((value >> 8) & 0xFF);
}
#endif

#if (USE_SWAP_UINT16 > 0)
uint16_t swap_uint16(uint16_t value)
{
  return (value << 8) | (value >> 8);
}
#endif

#if (USE_SWAP_INT32 > 0)
int32_t swap_int32(int32_t value)
{
  value = ((value << 8) & 0xFF00FF00) | ((value >> 8) & 0xFF00FF);
  return (value << 16) | ((value >> 16) & 0xFFFF);
}
#endif

#if (USE_SWAP_UINT32 > 0)
uint32_t swap_uint32(uint32_t value)
{
  value = ((value << 8) & 0xFF00FF00) | ((value >> 8) & 0xFF00FF);
  return (value << 16) | (value >> 16);
}
#endif

#if (USE_SWAP_INT64 > 0)
int64_t swap_int64(int64_t value)
{
  value = ((value << 8) & 0xFF00FF00FF00FF00ULL) | ((value >> 8) & 0x00FF00FF00FF00FFULL);
  value = ((value << 16) & 0xFFFF0000FFFF0000ULL) | ((value >> 16) & 0x0000FFFF0000FFFFULL);
  return (value << 32) | ((value >> 32) & 0xFFFFFFFFULL);
}
#endif

#if (USE_SWAP_UINT64 > 0)
uint64_t swap_uint64(uint64_t value)
{
  value = ((value << 8) & 0xFF00FF00FF00FF00ULL) | ((value >> 8) & 0x00FF00FF00FF00FFULL);
  value = ((value << 16) & 0xFFFF0000FFFF0000ULL) | ((value >> 16) & 0x0000FFFF0000FFFFULL);
  return (value << 32) | (value >> 32);
}
#endif

#if (USE_BIN2BCD > 0)
uint8_t bin2bcd(uint8_t value)
{
  return (((value / 10) << 4) + (value % 10));
}
#endif

#if (USE_BCD2BIN > 0)
uint8_t bcd2bin(uint8_t value)
{
  return (((value >> 4) * 10) + (value & 0x0F));
}
#endif

#if (USE_MAKEWORD > 0)
uint16_t makeword(uint8_t hb, uint8_t lb)
{
  return ((uint16_t)((((uint16_t)hb) << 8) | ((uint8_t)lb)));
}
#endif

#if (USE_MAKELONG > 0)
uint32_t makelong(uint16_t hw, uint16_t lw)
{
  return ((uint32_t)((((uint32_t)hw) << 16) | ((uint16_t)lw)));
}
#endif


#if 0
void itos(char *s, int32_t value, uint8_t radix, uint8_t minlen)
{
  char str[sizeof(unsigned long) * 8 + 1], i;
  char    *p=s;

  my_ltoa(value, (char *)str, radix);

  *p=0;
  if (minlen && (i = strlen((const char *)str)) < minlen)
  {
    while (i++ < minlen)
        *p++= '0';
  }
  *p=0;
  strcat(s,str);
}
#endif

/*
 * shell.h
 *
 *  Created on: 16 mrt. 2018
 *      Author: VM
 */

#ifndef SHELL_H_
#define SHELL_H_

#include "main.h"



// Command line size.
#define SHELL_CMDLINE_SIZE         (24+1)

// Number of history lines.
#define SHELL_CMDLINE_HIST_COUNT   (0)

// Time for an delayed ESC in ms.
#define ESC_DELAY                  (10)


// Virtual key codes.
enum
{
  // Regular keys.
  VK_BEL = '\a',
  VK_BS = '\b',
  VK_HT = '\t',
  VK_LF = '\n',
  VK_VT = '\v',
  VK_FF = '\f',
  VK_CR = '\r',
  VK_ESC = 0x1B,
  VK_DEL = 0x7F,
  // Function keys.
  VK_F1 = 0x80,
  VK_F2,
  VK_F3,
  VK_F4,
  VK_F5,
  VK_F6,
  VK_F7,
  VK_F8,
  VK_F9,
  VK_F10,
  VK_F11,
  VK_F12,
  // Navigation keys.
  VK_HOME,
  VK_INS,
  VK_END,
  VK_PGUP,
  VK_PGDN,
  VK_UPARROW,
  VK_DOWNARROW,
  VK_RIGHTARROW,
  VK_LEFTARROW,
  // Keypad keys.
  VK_NUMLOCK,
  VK_KP_SLASH,
  VK_KP_MULTIPLY,
  VK_KP_MINUS
};

typedef struct
{
  char *cmd;
  char *desc;
  void (*fxn)(char *param);
} cmd_tbl_t;

typedef struct
{
  uint8_t id;
  char *str;
  uint8_t size;
} line_obj_t;

typedef struct
{
  line_obj_t cmdline[SHELL_CMDLINE_HIST_COUNT + 1];
  line_obj_t *p_cmdline;
#if (SHELL_CMDLINE_HIST_COUNT > 0)
  uint8_t cmd_idx;
#endif
  line_obj_t tabline;
  uint8_t tab_idx;
} cmdline_obj_t;

// Public function prototypes.
void shell_open(void);
void shell_close(void);
void shell_process(void);
bool shell_getc(uint8_t *c);



#endif /* SHELL_H_ */

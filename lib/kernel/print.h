#ifndef __PRINT_H__
#define __PRINT_H__
#include "lib/stdint.h"

void put_char(uint8_t char_asci);
void put_str(char *message);
void put_int(uint32_t num); // 以16进制打印
void set_cursor(uint32_t cursor_pos);
void cls_screen(void);
#endif

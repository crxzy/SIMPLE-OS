#ifndef _PRINT_H__
#define _PRINT_H__
#include "stdint.h"

void put_char(uint8_t char_asci);
void put_str(char *message);
void put_int(uint32_t num); // 以16进制打印
void set_cursor(uint32_t cursor_pos);

#endif

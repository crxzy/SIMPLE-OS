#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__
#include "stdint.h"
// 中断处理函数
typedef void *intr_handler;
/* INTR_OFF 关中断,
 * INTR_ON  开中断 */
enum intr_status { // 中断状态
    INTR_OFF,
    INTR_ON
};

void idt_init(void);
enum intr_status intr_get_status(void);
enum intr_status intr_set_status(enum intr_status);
enum intr_status intr_enable(void);
enum intr_status intr_disable(void);
void register_handler(uint8_t vector_no, intr_handler function);
#endif

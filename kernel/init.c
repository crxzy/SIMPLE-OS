#include "init.h"
#include "interrupt.h"

void init_all() {
    idt_init();    
}
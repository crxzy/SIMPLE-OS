#include "lib/kernel/print.h"
#include "init.h"
#include "interrupt.h"
#include "thread/thread.h"
#include "userprog/process.h"
#include "device/keyboard.h"
#include "lib/kernel/stdio-kernel.h"
#include "device/ioqueue.h"
#include "lib/stdio.h"
#include "lib/user/syscall.h"

void thread1(void *args) {
    while(1) {
        enum intr_status old_status = intr_disable();
        char k = ioq_getchar(&kbd_buf);
        printk("%c", k);
        intr_set_status(old_status);
    }    
}

void user_process(void *args) {
    printf("test");
    void* addr = malloc(4);
    printf("malloc addr:%x\n", addr);
    free(addr);
    while(1);
}

int main() {
    put_str("kernel initing..:\n");
    init_all();
    intr_enable();

    thread_start("keyboard", 30, thread1, NULL);
    process_execute(user_process, "pro1");

    thread_block(TASK_BLOCKED);

    while(1);
    return 0;
}
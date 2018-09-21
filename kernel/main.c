#include "file.h"
#include "ide.h"
#include "init.h"
#include "interrupt.h"
#include "ioqueue.h"
#include "keyboard.h"
#include "print.h"
#include "process.h"
#include "shell.h"
#include "stdio-kernel.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"
#include "thread.h"

void init(void) {
    pid_t p = fork();
    if (p) {
        printf("father\n");
    } else {
        // printf("child\n");
        my_shell();
    }

    while (1)
        ;
}

int main() {
    put_str("kernel initing..:\n");
    init_all();

    // thread_start("keyboard", 30, thread1, NULL);
    // process_execute(user_process, "pro1");

    /*************    写入应用程序    *************/
    // uint32_t file_size = 6064;
    // uint32_t sec_cnt = DIV_ROUND_UP(file_size, 512);
    // struct disk *sda = &(channels.devices[0]);
    // void *prog_buf = sys_malloc(file_size);
    // ide_read(sda, 300, prog_buf, sec_cnt);
    // int32_t fd = sys_open("/prog", O_CREAT | O_RDWR);
    // if (fd != -1) {
    //     if (sys_write(fd, prog_buf, file_size) == -1) {
    //         printk("file write error!\n");
    //         while (1)
    //             ;
    //     }
    // }
    /*************    写入应用程序结束   *************/
    cls_screen();
    ioq_putchar(&kbd_buf, '\n');

    intr_enable();
    thread_block(TASK_BLOCKED);

    while (1)
        ;
    return 0;
}
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
        int status;
        pid_t child_pid;
        while (1) {
            child_pid = wait(&status);
        }
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
    // uint32_t file_size = 7048;
    // uint32_t sec_cnt = DIV_ROUND_UP(file_size, 512);
    // struct disk *sda = &(channels.devices[0]);
    // void *prog_buf = sys_malloc(file_size);
    // ide_read(sda, 300, prog_buf, sec_cnt);
    // int32_t fd = sys_open("/cat", O_CREAT | O_RDWR);
    // if (fd != -1) {
    //     if (sys_write(fd, prog_buf, file_size) == -1) {
    //         printk("file write error!\n");
    //         while (1)
    //             ;
    //     }
    // }
    /*************    写入应用程序结束   *************/
    // char *file1 = "\n\
    //               // uint32_t file_size = 7048;\n\
    //               // uint32_t sec_cnt = DIV_ROUND_UP(file_size, 512);\n\
    //               // struct disk *sda = &(channels.devices[0]);\n\
    //               // void *prog_buf = sys_malloc(file_size);\n\
    //               // ide_read(sda, 300, prog_buf, sec_cnt);\n\
    //               // int32_t fd = sys_open(\"/cat\", O_CREAT | O_RDWR);\n\
    //               // if (fd != -1) {\
    //               //     if (sys_write(fd, prog_buf, file_size) == -1) {\n\
    //               //         printk(\"file write error!\n\");\n\
    //               //         while (1)\n\
    //               //             ;\n\
    //               //     }\n\
    //               // }\n\
    //               \n";
    // int32_t fd = sys_open("/file1", O_CREAT | O_RDWR);
    // sys_write(fd, file1, strlen(file1));
    // sys_close(fd);

    cls_screen();
    printk("\n\n\n");
    ioq_putchar(&kbd_buf, '\n');

    // process_execute(test, "test");
    intr_enable();

    thread_exit(running_thread(), true);
    return 0;
}
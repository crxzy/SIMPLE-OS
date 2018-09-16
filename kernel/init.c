#include "init.h"
#include "interrupt.h"
#include "device/timer.h"
#include "memory.h"
#include "thread/thread.h"
#include "device/console.h"
#include "device/keyboard.h"
#include "userprog/tss.h"
#include "userprog/syscall-init.h"
#include "device/ide.h"
#include "fs/fs.h"

void init_all() {
    // 初始化中断
    idt_init();  
    // 初始化定时器  
    timer_init();
    // 内存管理
    mem_init();
    thread_init();   // 初始化县城
    console_init();  //控制台初始化
    keyboard_init(); //初始化键盘
    tss_init();
    syscall_init(); // 初始化系统调用
    ide_init();
    filesys_init();   // 初始化文件系统
}
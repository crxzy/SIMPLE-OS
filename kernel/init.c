#include "init.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "tss.h"
#include "syscall-init.h"
#include "ide.h"
#include "fs.h"

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
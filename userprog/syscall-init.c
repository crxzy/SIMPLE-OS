#include "syscall-init.h"
#include "device/console.h"
#include "lib/kernel/print.h"
#include "lib/stdint.h"
#include "lib/string.h"
#include "lib/user/syscall.h"
#include "thread/thread.h"

#define syscall_nr 32
typedef void *syscall;
syscall syscall_table[syscall_nr];

/* 返回当前任务的pid */
uint32_t sys_getpid(void) { return running_thread()->pid; }

void sys_write(void *buf) {
    console_put_str(buf);
}

/* 初始化系统调用 */
void syscall_init(void) {
    put_str("syscall_init start\n");
    syscall_table[SYS_GETPID] = sys_getpid;
    syscall_table[SYS_WRITE] = sys_write;
    syscall_table[SYS_MALLOC] = sys_malloc;
    syscall_table[SYS_FREE] = sys_free;
    put_str("syscall_init done\n");
}

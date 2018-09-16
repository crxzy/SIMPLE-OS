#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "lib/kernel/list.h"
#include "memory.h"
#include "stdint.h"

#define MAX_FILES_OPEN_PER_PROC 8

typedef void thread_func(void *);
typedef int16_t pid_t;

// 进程或线程的状态
enum task_status {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};

/***********   中断栈intr_stack   ***********/
struct intr_stack {
    uint32_t vec_no; // kernel.S 宏VECTOR中push %1压入的中断号
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t
        esp_dummy; // 虽然pushad把esp也压入,但esp是不断变化的,所以会被popad忽略
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t err_code; // err_code会被压入在eip之后
    void (*eip)(void);
    uint32_t cs;
    uint32_t eflags;
    void *esp;
    uint32_t ss;
};

/***********  线程栈thread_stack  ***********/
struct thread_stack {
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    /* 线程第一次执行时,eip指向待调用的函数kernel_thread
    其它时候,eip是指向switch_to的返回地址*/
    void (*eip)(thread_func *func, void *func_arg);

    /* 参数unused_ret只为占位置充数为返回地址 */
    void(*unused_retaddr);
    thread_func *function; // 由Kernel_thread所调用的函数名
    void *func_arg;        // 由Kernel_thread所调用的函数所需的参数
};

// 进程或线程的pcb,程序控制块 
struct task_struct {
    uint32_t *self_kstack; // 各内核线程都用自己的内核栈
    pid_t pid;
    enum task_status status;
    char name[16];
    uint8_t priority;
    uint8_t ticks; // 每次在处理器上执行的时间嘀嗒数

    /* 此任务自上cpu运行后至今占用了多少cpu嘀嗒数,
     * 也就是此任务执行了多久*/
    uint32_t elapsed_ticks;

    /* general_tag的作用是用于线程在一般的队列中的结点 */
    struct list_elem general_tag;

    /* all_list_tag的作用是用于线程队列thread_all_list中的结点 */
    struct list_elem all_list_tag;

    uint32_t *pgdir; // 进程自己页表的虚拟地址

    // 进程虚拟地址
    struct virtual_addr userprog_vaddr;
    // 内存块描述符
    struct mem_block_desc u_block_desc[DESC_CNT];

    // 文件描述符
    int32_t fd_table[MAX_FILES_OPEN_PER_PROC];
    uint32_t cwd_inode_nr;	 // 进程所在的工作目录的inode编号

    uint32_t stack_magic; // 检测栈的溢出
};

extern struct list thread_ready_list;
extern struct list thread_all_list;

void thread_create(struct task_struct *pthread, thread_func function,
                   void *func_arg);
void init_thread(struct task_struct *pthread, char *name, int prio);
struct task_struct *thread_start(char *name, int prio, thread_func function,
                                 void *func_arg);
struct task_struct *running_thread(void);
void schedule(void);
void thread_init(void);
void thread_block(enum task_status stat);
void thread_unblock(struct task_struct *pthread);
void thread_yield(void);
#endif

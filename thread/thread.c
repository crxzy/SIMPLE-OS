#include "thread.h"
#include "debug.h"
#include "global.h"
#include "interrupt.h"
#include "memory.h"
#include "lib/kernel/print.h"
#include "userprog/process.h"
#include "stdint.h"
#include "lib/string.h"
#include "thread/sync.h"

#define PG_SIZE 4096

struct task_struct *main_thread;     // 主线程PCB
struct list thread_ready_list;       // 就绪队列
struct list thread_all_list;         // 所有任务队列
static struct list_elem *thread_tag; // 用于保存队列中的线程结点
struct lock pid_lock;                // 分配pid锁
struct task_struct *idle_thread;     // idle线程

extern void switch_to(struct task_struct *cur, struct task_struct *next);

// 系统空闲时运行的线程 
static void idle(void *arg) {
    while (1) {
        thread_block(TASK_BLOCKED);
        asm volatile("sti; hlt" : : : "memory");
    }
}

// 主动让出cpu,换其它线程运行 
void thread_yield(void) {
    struct task_struct *cur = running_thread();
    enum intr_status old_status = intr_disable();
    ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
    list_append(&thread_ready_list, &cur->general_tag);
    cur->status = TASK_READY;
    schedule();
    intr_set_status(old_status);
}

// 获取当前线程pcb指针 
struct task_struct *running_thread() {
    uint32_t esp;
    asm("mov %%esp, %0" : "=g"(esp));
    // PCB和栈在同一4K空间中
    return (struct task_struct *)(esp & 0xfffff000);
}

// 由kernel_thread去执行function(func_arg) 
static void kernel_thread(thread_func *function, void *func_arg) {
    /* 开中断,避免中断被屏蔽,无法调度其它线程 */
    intr_enable();
    function(func_arg);
}

// 初始化线程栈thread_stack,将待执行的函数和参数放到thread_stack中相应的位置 
void thread_create(struct task_struct *pthread, thread_func function,
                   void *func_arg) {
    // 先预留中断栈空间
    pthread->self_kstack -= sizeof(struct intr_stack);

    // 留出线程栈空间
    pthread->self_kstack -= sizeof(struct thread_stack);
    struct thread_stack *kthread_stack =
        (struct thread_stack *)pthread->self_kstack;

    // 新建线程入口函数为 kernel_thread
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi =
        kthread_stack->edi = 0;
}

// 分配pid 
static pid_t allocate_pid(void) {
    static pid_t next_pid = 0;
    lock_acquire(&pid_lock);
    next_pid++;
    lock_release(&pid_lock);
    return next_pid;
}

// 初始化线程基本信息 
void init_thread(struct task_struct *pthread, char *name, int prio) {
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);

    // 刚创建的线程状态为 就绪
    pthread->status = TASK_READY;
    pthread->pid = allocate_pid();

    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + PG_SIZE);
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL;

    memset(pthread->fd_table, -1, sizeof(pthread->fd_table));

    pthread->fd_table[0] = 0;
    pthread->fd_table[1] = 1;
    pthread->fd_table[2] = 2;
    pthread->stack_magic = 0x19970119; // 自定义的魔数
}

// 创建优先级为prio的线程,线程名为name,线程所执行的函数是function(func_arg) 
struct task_struct *thread_start(char *name, int prio, thread_func function, void *func_arg) {
    // PCB
    struct task_struct *thread = get_kernel_pages(1);

    init_thread(thread, name, prio);
    thread_create(thread, function, func_arg);

    // 加入就绪队列
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);

    //加入thread_all_list
    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);

    return thread;
}

// 添加main线程的PCB 
static void make_main_thread(void) {
    // 内核栈为0XC009F000 PCB为0XC009E000
    main_thread = running_thread();
    init_thread(main_thread, "main", 30);
    main_thread->status = TASK_RUNNING;

    // 当前就是main线程， 加入thread_all_list队列
    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

// 任务调度 
void schedule() {
    ASSERT(intr_get_status() == INTR_OFF);

    struct task_struct *cur = running_thread();
    if (cur->status == TASK_RUNNING) { // 若此线程只是cpu时间片到了,将其加入到就绪队列尾
        ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
        list_append(&thread_ready_list, &cur->general_tag);
        cur->ticks = cur->priority; // 重新将当前线程的ticks再重置为其priority;
        cur->status = TASK_READY;
    } else {
        if (cur->status == TASK_BLOCKED) {
        }
    }

    if (list_empty(&thread_ready_list)) {
        thread_unblock(idle_thread);
    }

    ASSERT(!list_empty(&thread_ready_list));
    thread_tag = NULL;
    // 就绪队列取出下一个进程
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct *next = elem2entry(struct task_struct, general_tag, thread_tag);
    // 设置状态为运行，并切换线程
    next->status = TASK_RUNNING;
    // 激活页表
    process_activate(next);
    switch_to(cur, next);
}

// 将当前线程阻塞,标志其状态为stat. 
void thread_block(enum task_status stat) {
    ASSERT(((stat == TASK_BLOCKED) || (stat == TASK_WAITING) ||
            (stat == TASK_HANGING)));
    enum intr_status old_status = intr_disable();
    struct task_struct *cur_thread = running_thread();
    cur_thread->status = stat;
    schedule();
    intr_set_status(old_status);
}

// 将线程pthread解除阻塞
void thread_unblock(struct task_struct *pthread) {
    enum intr_status old_status = intr_disable();
    ASSERT(((pthread->status == TASK_BLOCKED) ||
            (pthread->status == TASK_WAITING) ||
            (pthread->status == TASK_HANGING)));
    if (pthread->status != TASK_READY) {
        ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
        if (elem_find(&thread_ready_list, &pthread->general_tag)) {
            PANIC("thread_unblock: blocked thread in ready_list\n");
        }
        list_push(&thread_ready_list, &pthread->general_tag);
        pthread->status = TASK_READY;
    }
    intr_set_status(old_status);
}

// 初始化线程环境 
void thread_init(void) {
    put_str("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    lock_init(&pid_lock);
    make_main_thread();
    idle_thread = thread_start("idle", 10, idle, NULL);

    put_str("thread_init done\n");
}

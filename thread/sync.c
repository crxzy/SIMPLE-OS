#include "sync.h"
#include "debug.h"
#include "global.h"
#include "i386/cmpxchg.h"
#include "interrupt.h"
#include "list.h"
#include "print.h"

// 初始化信号量 
void sema_init(struct semaphore *psema, uint8_t value) {
    psema->value = value;
    psema->spin_value = 0;
    list_init(&psema->waiters);
}

// 初始化锁plock
void lock_init(struct lock *plock) {
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_init(&plock->semaphore, 1); // 信号量初值为1
}

// 自旋锁
void spin_lock(struct semaphore *psema) {
    while (!cmpxchg((uint32_t *)(&(psema->spin_value)), 0, 1))
        ;
    // put_char('\n');put_int(psema->preempt);put_char('\n');
}

void spin_unlock(struct semaphore *psema) { psema->spin_value = 0; }

// 信号量down操作/P
void sema_down(struct semaphore *psema) {
    /*
    enum intr_status old_status = intr_disable();
    //spin_lock(psema);
    //信号量值为0，放入等待队列，阻塞自己
    while (psema->value == 0)
    {
          ASSERT(!elem_find(&psema->waiters, &running_thread()->general_tag));
          if (elem_find(&psema->waiters, &running_thread()->general_tag))
          {
                PANIC("sema_down: thread blocked has been in waiters_list\n");
          }
          list_append(&psema->waiters, &running_thread()->general_tag);
          thread_block(TASK_BLOCKED); // 阻塞线程,直到被唤醒
    }

    //信号量不为0，减1，即获得锁
    psema->value--;
    ASSERT(psema->value == 0);

    intr_set_status(old_status);
    */

    enum intr_status old_status = intr_disable();
    // spin_lock(psema);
    while (psema->value <= 0) {
        list_append(&psema->waiters, &running_thread()->general_tag);
        thread_block(TASK_BLOCKED); // 阻塞线程,直到被唤醒
    }
    psema->value--;
    // spin_lock(psema);
    intr_set_status(old_status);
}

// 信号量的up操作/V
void sema_up(struct semaphore *psema) {
    /*
    enum intr_status old_status = intr_disable();
    ASSERT(psema->value == 0);
    if (!list_empty(&psema->waiters))
    {
          struct task_struct *thread_blocked = elem2entry(struct task_struct,
    general_tag, list_pop(&psema->waiters)); thread_unblock(thread_blocked);
    }
    psema->value++;
    ASSERT(psema->value == 1);

    intr_set_status(old_status);*/
    enum intr_status old_status = intr_disable();
    // spin_unlock(psema);
    if (!list_empty(&psema->waiters)) {
        struct task_struct *thread_blocked = elem2entry(
            struct task_struct, general_tag, list_pop(&psema->waiters));
        thread_unblock(thread_blocked);
    }
    psema->value++;
    intr_set_status(old_status);
}

// 获取锁
void lock_acquire(struct lock *plock) {
    // 如何holder已经持有锁，holder_repeat_nr++
    if (plock->holder != running_thread()) {
        sema_down(&plock->semaphore); // P操作
        plock->holder = running_thread();
        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1;
    } else {
        plock->holder_repeat_nr++;
    }
    // put_int(plock->holder_repeat_nr);put_char(' ');
}

// 释放锁 
void lock_release(struct lock *plock) {
    ASSERT(plock->holder == running_thread());
    // 多次申请锁的情况
    if (plock->holder_repeat_nr > 1) {
        plock->holder_repeat_nr--;
        return;
    }
    ASSERT(plock->holder_repeat_nr == 1);

    plock->holder = NULL; // 把锁的持有者置空放在V操作之前
    plock->holder_repeat_nr = 0;
    sema_up(&plock->semaphore); // V操作

    // put_int(plock->holder_repeat_nr);put_char(' ');
}

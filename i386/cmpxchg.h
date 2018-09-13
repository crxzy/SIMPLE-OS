#ifndef __CMPXCHG_H_
#define __CMPXCHG_H_
#include "global.h"
#include "stdint.h"

static inline bool cmpxchg(uint32_t *ptr, uint32_t old, uint32_t new) {
    uint32_t ret;
    asm volatile(LOCK_PREFIX "cmpxchgl %2,%1"
                 : "=a"(ret), "+m"(*ptr)
                 : "r"(new), "0"(old)
                 : "memory");
    return ret == old;
}

#endif
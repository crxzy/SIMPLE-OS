#ifndef __DEBUG_H__
#define __DEBUG_H__
void panic_spin(char *filename, int line, const char *func,
                const char *condition);

#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef DEBUG
#define ASSERT(CONDITION)                                                      \
    if (CONDITION) {                                                           \
    } else {                                                                   \
        /* 符号#让编译器将宏的参数转化为字符串字面量 */    \
        PANIC(#CONDITION);                                                     \
    }
#else
#define ASSERT(CONDITION) ((void)0)
#endif

#endif

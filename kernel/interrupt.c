#include "interrupt.h"
#include "global.h"
#include "print.h"
#include "io.h"

// 8259A终端控制器
#define PIC_M_CTRL 0x20 // 主片控制端口0x20
#define PIC_M_DATA 0x21 // 主片数据端口0x21
#define PIC_S_CTRL 0xa0 // 从片控制端口0xa0
#define PIC_S_DATA 0xa1 // 从片数据端口0xa1

#define IDT_DESC_CNT 0x81 // 中断数

#define EFLAGS_IF 0x00000200 // eflags寄存器中的if位为1
#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl; popl %0" : "=g"(EFLAG_VAR))

// 中断门描述符结构体
struct gate_desc {
    uint16_t func_offset_low_word;
    uint16_t selector;
    uint8_t dcount;    //全0
    uint8_t attribute; // P DEL S TYPE
    uint16_t func_offset_high_word;
};

// 中断门描述符数组
static struct gate_desc idt[IDT_DESC_CNT] = {0}; 
// 异常的名字
char *intr_name[IDT_DESC_CNT];         
// 真正处理异常的函数
intr_handler idt_table[IDT_DESC_CNT] = {0};      
// 中断处理函数入口数组
extern intr_handler intr_entry_table[IDT_DESC_CNT]; 
extern uint32_t syscall_handler();

// 创建中断门描述符
static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr,
                          intr_handler function) {
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

// 初始化中断描述符
static void idt_desc_init() {
    int i;
    for (i = 0; i < 0x30; i++) {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }

    // syscall
    make_idt_desc(&idt[0x80], IDT_DESC_ATTR_DPL3, syscall_handler);
    put_str("   idt_desc_init done\n");
}

// 初始化8259A
static void pic_init(void) {
    // 初始化主片 
    outb(PIC_M_CTRL, 0x11); // ICW1: 边沿触发,级联8259, 需要ICW4.
    outb(PIC_M_DATA,
         0x20); // ICW2: 起始中断向量号为0x20,也就是IR[0-7] 为 0x20 ~ 0x27.
    outb(PIC_M_DATA, 0x04); // ICW3: IR2接从片.
    outb(PIC_M_DATA, 0x01); // ICW4: 8086模式, 正常EOI

    // 初始化从片
    outb(PIC_S_CTRL, 0x11); // ICW1: 边沿触发,级联8259, 需要ICW4.
    outb(PIC_S_DATA,
         0x28); // ICW2: 起始中断向量号为0x28,也就是IR[8-15] 为 0x28 ~ 0x2F.
    outb(PIC_S_DATA, 0x02); // ICW3: 设置从片连接到主片的IR2引脚
    outb(PIC_S_DATA, 0x01); // ICW4: 8086模式, 正常EOI

/*
    outb(PIC_M_DATA, 0xf8);
    outb(PIC_S_DATA, 0xbf);
*/
    outb(PIC_M_DATA, 0x0);
    outb(PIC_S_DATA, 0x0);
    /*
    //打开主片上IR0,也就是目前只接受时钟产生的中断
    outb(PIC_M_DATA, 0xfe);
    outb(PIC_S_DATA, 0xff);*/

    /*
    //测试键盘,只打开键盘中断，其它全部关闭
    outb (PIC_M_DATA, 0xfd);
    outb (PIC_S_DATA, 0xff);*/

    put_str("   pic_init done\n");
}

// 默认中断处理函数
static void general_intr_handler(uint8_t vec_nr) {
    static int i = 0;
    if (vec_nr == 0x27 || vec_nr == 0x2f) {
        return; // IRQ7和IRQ15的中断无须处理。
    }
    /*
    if (vec_nr == 0xd) {
        asm ("nop");
            2018年8月17日02点31分
            莫名发生#GP异常
            经调试发现ERROR CODE为0X13B
            #GP异常错误代码为选择器错误代码(Selector Error Code)

            # https://wiki.osdev.org/Exceptions#Selector_Error_Code
            Selector Error Code
            31         16   15         3   2   1   0
            +---+--  --+---+---+--  --+---+---+---+---+
            |   Reserved   |    Index     |  Tbl  | E |
            +---+--  --+---+---+--  --+---+---+---+---+
            Length	Name	Description
            E	1 bit	External	When set, the exception originated
    externally to the processor. Tbl	2 bits	IDT/GDT/LDT table
    This is one of the following values: Value	Description
            0b00	The Selector Index references a descriptor in the GDT.
            0b01	The Selector Index references a descriptor in the IDT.
            0b10	The Selector Index references a descriptor in the LDT.
            0b11	The Selector Index references a descriptor in the IDT.
            Index	13 bits	Selector Index	The index in the GDT, IDT or
    LDT.

            0X13B E位为1
            异常来自处理器外部，忽略之

    }*/
    // 输出信息
    /*
    put_str("int vector: 0x");
    put_int(vec_nr);
    put_char(' ');
    put_str(intr_name[vec_nr]);
    put_str(" times: 0x");
    put_int(++i);
    put_str("\n");
    */
    /* 将光标置为0,从屏幕左上角清出一片打印异常信息的区域,方便阅读 */
    set_cursor(0);
    int cursor_pos = 0;
    while (cursor_pos < 320) {
        put_char(' ');
        cursor_pos++;
    }

    set_cursor(0); // 重置光标为屏幕左上角
    put_str("!!!!!!!      EXCEPTION ERROR:  !!!!!!!!\n");
    set_cursor(88); // 从第2行第8个字符开始打印
    put_str(intr_name[vec_nr]);
    if (vec_nr == 14) { // 若为Pagefault,将缺失的地址打印出来并悬停
        uint32_t page_fault_vaddr = 0;
        asm("movl %%cr2, %0"
            : "=r"(page_fault_vaddr)); // cr2是存放造成page_fault的地址
        put_str("\npage fault addr is ");
        put_int(page_fault_vaddr);
    }
    put_str("\n");
    while (1)
        ;
}

// 注册异常名字和处理程序的默认处理函数
static void exception_init(void) {
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++) {
        idt_table[i] = general_intr_handler; // 默认为general_intr_handler。
        intr_name[i] = "unknown";
    }
    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exception";
    intr_name[2] = "NMI Interrupt";
    intr_name[3] = "#BP Breakpoint Exception";
    intr_name[4] = "#OF Overflow Exception";
    intr_name[5] = "#BR BOUND Range Exceeded Exception";
    intr_name[6] = "#UD Invalid Opcode Exception";
    intr_name[7] = "#NM Device Not Available Exception";
    intr_name[8] = "#DF Double Fault Exception";
    intr_name[9] = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    // intr_name[15] 第15项是intel保留项，未使用
    intr_name[16] = "#MF x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
}

// 获取中断状态
enum intr_status intr_get_status() {
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    return (EFLAGS_IF & eflags) ? INTR_ON : INTR_OFF;
}

// 开中断、返回原状态
enum intr_status intr_enable() {
    enum intr_status old_status = intr_get_status();
    if(old_status == INTR_OFF) {
        asm volatile("sti" : : : "memory"); // sti将IF位置1
    }
    return old_status;
}

// 关中断
enum intr_status intr_disable() { 
    enum intr_status old_status = intr_get_status();
    if(old_status == INTR_ON) {
        asm volatile("cli" : : : "memory"); // cli指令将IF位置0
    }
    return old_status;
}

// 设置中断状态
enum intr_status intr_set_status(enum intr_status status) {
    return status & INTR_ON ? intr_enable() : intr_disable();
}

// 注册中断函数
void register_handler(uint8_t vector_no, intr_handler function) {
    // intr_entry_table[vector_no] 会调用 idt_table[vector_no]
    idt_table[vector_no] = function;
}


// 中断初始化
void idt_init() {
    put_str("idt_init start\n");
    idt_desc_init(); 
    exception_init();
    pic_init(); 

    /* 加载idt */
    // limit BASE
    uint64_t idt_operand =
        ((sizeof(idt) - 1) | ((uint64_t)(uint32_t)idt << 16));
    asm volatile("lidt %0" : : "m"(idt_operand));
    put_str("idt_init done\n");
}
[bits 32]
section .text
global switch_to
switch_to:
    ; next
    ; cur
    ;栈中此处是返回地址	       
    push esi
    push edi
    push ebx
    push ebp

    ; 保存当前线程的栈指针
    mov eax, [esp + 20]		 ; 得到栈中的参数cur, cur = [esp+20]
    mov [eax], esp           ; task_struct 前四字节为栈指针

    ; 切换到下一个线程的栈空间
    mov eax, [esp + 24]		 ; 得到栈中的参数next, next = [esp+24]
    mov esp, [eax]		
                   
    pop ebp
    pop ebx
    pop edi
    pop esi
    ret				 ; 从switch_to返回， 第一次执行时会返回到kernel_thread
                   

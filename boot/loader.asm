%include 'boot.inc'

SECTION loader vstart=LOADER_BASE_ADDR
PROGRAM_LENGTH:
            dd PROGRAM_END-PROGRAM_LENGTH
ENTRY_POINT:
            dw start-PROGRAM_LENGTH
KERNEL_ENTRY:
            dd 0
            
GDT_BASE:   dd 0
            dd 0

CODE_DESC:  dd 0x0000ffff
            dd DESC_CODE_HIGH4

DATA_STACK_DESC:
            dd 0x0000ffff
            dd DESC_DATA_HIGH4

VIDEO_DESC:
            ; limit = (0xbffff-0xb8000)/4k=7
            dd 0x80000007
            dd DESC_VICEO_HIGH4

GDT_SIZE    equ $-GDT_BASE
GDT_LIMIT   equ GDT_SIZE-1

times 100 dq 0


ARDS_BUF:    times 200 db 0
ARDS_NR:     dw 0


GDT_PTR:    dw GDT_LIMIT
            dd GDT_BASE

SELECTOR_CODE   equ (1<<3) + TI_GDT + RPL0
SELECTOR_DATA   equ (2<<3) + TI_GDT + RPL0
SELECTOR_VIDEO  equ (3<<3) + TI_GDT + RPL0

start:
    mov ax, 0
    mov ds, ax
    mov sp, LOADER_BASE_ADDR

    ; VBE
    ; 143 800x600 16.8M(8:8:8) 0xe0000000
    ;mov ax, 0x4f02
    ;mov bx, 0x4143
    ;int 10h

    ; 获取物理内存容量
    ; int 15h eax=0xe820 edx=0x534D4150('SMAP')
    xor ebx, ebx
    mov edx, 0x534d4150
    mov di, ARDS_BUF
@loop:
    mov eax, 0xe820
    mov ecx, 20
    int 0x15
    jc @o

    add di, cx
    inc word [ARDS_NR]
    cmp ebx, 0
    jnz @loop

    mov cx, [ARDS_NR]
    mov ebx, ARDS_BUF
    xor edx, edx
@find:
    mov eax, [ebx]
    add eax, [ebx+8]
    add ebx, 20

    cmp edx, eax
    jge @next_ards
    mov edx, eax
@next_ards:
    loop @find
    mov [LOADER_BASE_ADDR], edx
@o:
    
    ; A20 GATE
    in al, 0x92
    or al, 10b
    out 0x92, al

    ; gdt
    lgdt [GDT_PTR]

    ; cr0
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp SELECTOR_CODE:p_mode_start

[bits 32]
p_mode_start:
    mov ax, SELECTOR_VIDEO
    mov gs, ax
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, LOADER_BASE_ADDR

    ; 创建目录项页表
    call setup_page
    ;call vbe_init

    sgdt [GDT_PTR]

    mov ebx, [GDT_PTR + 2]
    or dword [ebx + 0x18 + 4], 0xc0000000

    add dword [GDT_PTR + 2], 0xc0000000

    add esp, 0xc0000000

    mov eax, PAGE_DIR_TABLE_POS
    mov cr3, eax

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    lgdt [GDT_PTR]
    
    ; 读取kernel
    call kernel_init
    mov esp, 0xc009f000
    ;jmp word [KERNEL_ENTRY]  
    ; 2018年9月3日18点46分。。。
    ; 查了半天BUG， word导致后面地址(PF)的错误
    jmp dword [KERNEL_ENTRY]  

    ;hlt

; 低端1MB映射到0XC0000000-0XC00FFFFF
; 创建高1G的目录项
setup_page:
    ;PAGE
    ;mov ecx, 4096
    mov ecx, 0x100000
    mov esi, 0
.clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS + esi], 0
    inc esi
    loop .clear_page_dir

.create_pde:
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x1000
    ; 0x101000
    mov ebx, eax

    or eax, PG_US_U | PG_RW_W | PG_P
    mov [PAGE_DIR_TABLE_POS], eax
    mov [PAGE_DIR_TABLE_POS + 0xc00], eax
    
    sub eax, 0x1000
    mov [PAGE_DIR_TABLE_POS + 4092], eax

    mov ecx, 256
    mov esi, 0
    xor edx, edx
    mov edx, PG_US_U | PG_P | PG_RW_W
.create_pte:
    mov [ebx + esi*4], edx
    add edx, 4096
    inc esi
    loop .create_pte

    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x2000
    ; 0x102000
    or eax, PG_US_U | PG_P | PG_RW_W
    mov ebx, PAGE_DIR_TABLE_POS
    mov ecx, 254
    mov esi, 769
.create_kernel_pde:
    mov [ebx + esi*4], eax
    inc esi
    add eax, 0x1000
    loop .create_kernel_pde
    ret

; 0XE000 0000 - 0XE01D 4C00
; 0X1D4C00/0X1000=468
; 0xe00-0xc00 * 0x400 = 0x181000
vbe_init:
    mov ecx, 468
    mov esi, 0
    mov edx, 0x181000
    mov ebx, 0xe0000000
    or ebx, PG_US_U | PG_P | PG_RW_W
.vbe_l:
    mov [edx + esi*4], ebx
    inc esi
    add ebx, 0x1000
    loop .vbe_l
    ret


kernel_init:
;----------------------------------
; 读取kernel到内存(KERNEL_BASE_ADDR)
;----------------------------------
    ;mov esp, 0xc009f000
    mov ecx, 200
    xor di, di
    mov si, KERNEL_START_SECTOR
    mov ebx, KERNEL_BASE_ADDR
.read_kernel:
    call read_hard_disk_m32
    inc si
    loop .read_kernel
;----------------------------------
; 初始化kernel
;----------------------------------
    ;kernel_entry
    mov eax, [KERNEL_BASE_ADDR + 24]
    mov [KERNEL_ENTRY], eax

    xor eax, eax
    xor ebx, ebx		;程序头表地址
    xor ecx, ecx		;程序头表program header数量
    xor edx, edx		;program header尺寸,即e_phentsize

    mov dx, [KERNEL_BASE_ADDR + 42]	    ; 偏移文件42字节处的属性是e_phentsize,表示program header大小
    mov ebx, [KERNEL_BASE_ADDR + 28]    ; 偏移文件开始部分28字节的地方是e_phoff program header在文件中的偏移量
                        
    add ebx, KERNEL_BASE_ADDR
    mov cx, [KERNEL_BASE_ADDR + 44]     ; 偏移文件开始部分44字节的地方是e_phnum,有几个program header
.each_segment:
    cmp byte [ebx + 0], PT_NULL		        ; 若p_type等于 PT_NULL,说明此program header未使用。
    je .PTNULL

    push dword [ebx + 16]		            ; program header中偏移16字节的地方是p_filesz,压入函数memcpy的第三个参数:size
    mov eax, [ebx + 4]			            ; 距程序头偏移量为4字节的位置是p_offset
    add eax, KERNEL_BASE_ADDR	        ; 加上kernel.bin被加载到的物理地址,eax为该段的物理地址
    push eax				                ; 压入函数memcpy的第二个参数:源地址
    push dword [ebx + 8]			        ; 压入函数memcpy的第一个参数:目的地址,偏移程序头8字节的位置是p_vaddr，这就是目的地址
    call mem_cpy				            ; 调用mem_cpy完成段复制
    add esp,12				                ;清理栈中压入的三个参数
.PTNULL:
    add ebx, edx				            ;edx为program header大小,即e_phentsize,在此ebx指向下一个program header 
    loop .each_segment


;----------------------------------
; FUNCTIONS
;----------------------------------
read_hard_disk_m32:                        ;从硬盘读取一个逻辑扇区
                                         ;输入: DI:SI=起始逻辑扇区号
                                         ;      DS:EBX=目标缓冲区地址
    push eax
    push ecx
    push edx

    mov dx, 0x1f2
    mov al, 1
    out dx, al          ; 读取的扇区数

    inc dx              
    mov ax, si
    out dx, al          ; LBA 7-0

    inc dx
    mov al, ah
    out dx, al          ; LBA 15-8

    inc dx
    mov ax, di
    out dx, al

    inc dx
    mov al, 0xe0        ; LBA 24-27   1110[LBA24-27]
    or al, ah
    out dx, al

    inc dx              ; 0x1f7 写时command 读时status
    mov al, 0x20
    out dx, al

.waits:
    in al, dx
    and al, 0x88
    cmp al, 0x08
    jnz .waits

    mov ecx, 256
    mov dx, 0x1f0

.readw:
    in ax, dx
    mov [ebx], ax
    add ebx, 2
    loop .readw
    
    pop edx
    pop ecx
    pop eax
    ret

;----------  逐字节拷贝 mem_cpy(dst,src,size) ------------
;输入:栈中三个参数(dst,src,size)
;输出:无
;---------------------------------------------------------
    mem_cpy:		      
    cld
    push eax
    push ebp
    mov ebp, esp
    push ecx		   ; rep指令用到了ecx，但ecx对于外层段的循环还有用，故先入栈备份
    mov edi, [ebp + 12]	   ; dst
    mov esi, [ebp + 16]	   ; src
    mov ecx, [ebp + 20]	   ; size
    rep movsb		   ; 逐字节拷贝

    ;恢复环境
    pop ecx		
    pop ebp
    pop eax
    ret

PROGRAM_END:
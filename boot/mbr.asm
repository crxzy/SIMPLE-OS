%include 'boot.inc'

loader_lba equ 1

SECTION MBR vstart=0x7c00
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov ss, ax
    mov sp, 0x7c00

    ;清屏
    ;mov ax, 0x600
    ;mov bx, 0x700
    ;mov cx, 0
    ;mov dx, 0x184f
    ;int 0x10

    ; phy_base / 16
    mov ax, [loader_addr]
    mov dx, [loader_addr+2]
    mov bx, 16
    div bx
    mov ds, ax
    
    xor di, di
    mov si, loader_lba
    xor bx, bx
    ; 读取LBA为 loader_lba 的数据到  loader_addr
    call read_hard_disk_0

    ; dd PROGRAM_LENGTH
    ; dw ENTRY_POINT

    ; PROGRAM_LENGTH / 512
    push bx
    
    mov ax, [0]
    mov dx, [2]
    mov bx, 512
    div bx

    pop bx

    cmp dx, 0
    jnz @1
    dec ax

@1:
    cmp ax, 0
    jz @end
    mov cx, ax

@2:
    inc si
    call read_hard_disk_0
    loop @2

@end:

    mov bx, [4]
    ; 跳转到 DS:ENTRY_POINT
    push ds
    push bx
    retf
    



;----------------------------------------------------------------
read_hard_disk_0:                        ;从硬盘读取一个逻辑扇区
                                         ;输入: DI:SI=起始逻辑扇区号
                                         ;      DS:BX=目标缓冲区地址
    push ax
    push cx
    push dx

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

    mov cx, 256
    mov dx, 0x1f0

.readw:
    in ax, dx
    mov [bx], ax
    add bx, 2
    loop .readw
    
    pop dx
    pop cx
    pop ax

    ret

    loader_addr dd LOADER_BASE_ADDR
    times 510-($-$$) db 0
    dw 0xaa55
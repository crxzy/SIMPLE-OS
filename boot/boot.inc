LOADER_BASE_ADDR    equ    0x900
KERNEL_START_SECTOR equ    0x9
KERNEL_BASE_ADDR    equ    0x70000
KERNEL_ENTRY_POINT  equ    0xc0001500

PT_NULL     equ     0x0
; G=1
DESC_G_4K   equ     0x80_0000
; D=1
DESC_D_32   equ     0x40_0000
; P=1
DESC_P      equ     0x00_8000
; S
DESC_S_NOR  equ     0x00_1000
DESC_S_SYS  equ     0x00_0000

DESC_DPL_0  equ     0x00_0000
DESC_DPL_1  equ     0x00_2000
DESC_DPL_2  equ     0x00_4000
DESC_DPL_3  equ     0x00_6000

; x=1 c=0 r=0 a=0
DESC_TYPE_CODE equ  0x00_0800
; X=0 E=0 W=1 A=0
DESC_TYPE_DATA equ  0x00_0200

DESC_LIMIT  equ     0x0f_0000

;--------------------------------------------
; 段描述符
; 段基址 ; G ; D/B ; L ; AVL ; 段界限 ; P ; DPL ; S ; TYPE ; 段基址
; 段基址                                  ; 段界限

DESC_CODE_HIGH4  equ DESC_G_4K + DESC_D_32 + DESC_LIMIT + DESC_P + DESC_DPL_0 + DESC_S_NOR + DESC_TYPE_CODE
DESC_DATA_HIGH4  equ DESC_G_4K + DESC_D_32 + DESC_LIMIT + DESC_P + DESC_DPL_0 + DESC_S_NOR + DESC_TYPE_DATA
DESC_VICEO_HIGH4 equ DESC_G_4K + DESC_D_32 + DESC_P + DESC_DPL_0 + DESC_S_NOR + DESC_TYPE_DATA + 0xb


RPL0 equ     00b
RPL1 equ     01b
RPL2 equ     10b
RPL3 equ     11b

TI_GDT equ  000b
TI_LDT equ  100b

;---------------------------------------------
; PDE
; 地址 31-12; AVL ; G ; 0 ; D ; A ; PCD ; PWT ; US ; RW ;P
; PTE
; 地址 31-12; AVL ; G ; PAT ; D ; A ; PCD ; PWT ; US ; RW ;P
; CR3
; 地址 31-12;  ;  PCD ; PWT ; ; ; ;
PAGE_DIR_TABLE_POS equ  0x100000
PG_P    equ    1b
PG_RW_R equ    00b
PG_RW_W equ    10b
PG_US_S equ    000b
PG_US_U equ    100b
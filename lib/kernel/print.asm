TI_GDT  equ   0
RPL0    equ   0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

section .data
put_int_buffer    dq    0

[bits 32]
section .text

;--------------------------------------------
;put_str 通过put_char来打印以0字符结尾的字符串
;--------------------------------------------
;输入：栈中参数为打印的字符串
;输出：无

global put_str
put_str:
   push ebx
   push ecx
   xor ecx, ecx		      
   mov ebx, [esp + 12]	      ; 从栈中得到待打印的字符串地址 
.goon:
   mov cl, [ebx]
   cmp cl, 0		      
   jz .str_over
   push ecx		      
   call put_char
   add esp, 4		     
   inc ebx		      
   jmp .goon
.str_over:
   pop ecx
   pop ebx
   ret

;------------------------   put_char   -----------------------------
;把栈中的1个字符写入光标所在处
;-------------------------------------------------------------------   
global put_char
put_char:
   pushad	 
   mov ax, SELECTOR_VIDEO	      
   mov gs, ax

   ;获取当前光标位置
   ;获得高8位
   mov dx, 0x03d4  ;索引寄存器
   mov al, 0x0e	   ;用于提供光标位置的高8位
   out dx, al
   mov dx, 0x03d5  
   in al, dx	  
   mov ah, al

   ;获取低8位
   mov dx, 0x03d4
   mov al, 0x0f
   out dx, al
   mov dx, 0x03d5 
   in al, dx

   ;将光标存入bx
   mov bx, ax	  
  
   mov ecx, [esp + 36]	      ;pushad压4×8＝32,返回地址4,esp+36
   cmp cl, 0xd				  ;\r\n 0D0A
   jz .is_carriage_return
   cmp cl, 0xa
   jz .is_line_feed

   cmp cl, 0x8				  ;backspace 8
   jz .is_backspace
   jmp .put_other	   
;;;;;;;;;;;;;;;;;;

 .is_backspace:		      
   dec bx
   shl bx,1
   mov byte [gs:bx], 0x20		  ;删除的字节补为' '
   inc bx
   mov byte [gs:bx], 0x07
   shr bx,1
   jmp .set_cursor
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

 .put_other:
   shl bx, 1				  ;光标值乘2=显存中的偏移字节
   mov [gs:bx], cl
   inc bx
   mov byte [gs:bx],0x07		
   shr bx, 1				  
   inc bx
   cmp bx, 2000		   
   jl .set_cursor			 ; 80X25=2000


 .is_line_feed:				  
 .is_carriage_return:			  
					  ; \r\n
   xor dx, dx				  
   mov ax, bx				  
   mov si, 80				  
   div si				  
   sub bx, dx				  

 .is_carriage_return_end:                 
   add bx, 80
   cmp bx, 2000
 .is_line_feed_end:		
   jl .set_cursor

;1-24->0-23
 .roll_screen:				  ; 滚屏
   cld  
   mov ecx, 960				  ;2000-80=1920 1920*2=3840 3840/4=960次
   ; 2018年8月29日02点20分。。。
   ; GP异常， 调试的好惨。。。。
   ;mov esi, 0xb80a0			 
   ;mov edi, 0xb8000			
   mov esi, 0xc00b80a0			  
   mov edi, 0xc00b8000			  
   rep movsd				  

  ;最后一行填充为空白
   mov ebx, 3840			  
   mov ecx, 80                            
 .cls:
   mov word [gs:ebx], 0x0720              
   add ebx, 2
   loop .cls 
   mov bx,1920				  ;将光标值重置为1920

 .set_cursor:   
					  ;将光标设为bx值
;;;;;;; 1 设置高8位 ;;;;;;;;
   mov dx, 0x03d4			  
   mov al, 0x0e				  
   out dx, al
   mov dx, 0x03d5			  
   mov al, bh
   out dx, al

;;;;;;; 2 设置低8位 ;;;;;;;;;
   mov dx, 0x03d4
   mov al, 0x0f
   out dx, al
   mov dx, 0x03d5 
   mov al, bl
   out dx, al
 .put_char_done: 
   popad
   ret


;------------------------------------------------------------------------------------------
global put_int
put_int:
   pushad
   mov ebp, esp
   mov eax, [ebp+4*9]		       ; 返回地址占4字节 pushad的8个4字节
   mov edx, eax
   mov edi, 7                          ; 指定在put_int_buffer中初始的偏移量
   mov ecx, 8			       ; 32位数字中,16进制数字的位数是8个
   mov ebx, put_int_buffer

.16based_4bits:			       
   and edx, 0x0000000F		      
   cmp edx, 9			      
   jg .is_A2F 
   add edx, '0'			      
   jmp .store
.is_A2F:
   sub edx, 10			       
   add edx, 'A'

.store:
   mov [ebx+edi], dl		       
   dec edi
   shr eax, 4
   mov edx, eax 
   loop .16based_4bits

.ready_to_print:
   inc edi			       ; edi为-1,加1使其为0
.skip_prefix_0:  
   cmp edi,8			       ; 若已经比较第9个字符了，表示待打印的字符串为全0 
   je .full0 
;找出连续的0字符, edi做为非0的最高位字符的偏移
.go_on_skip:   
   mov cl, [put_int_buffer+edi]
   inc edi
   cmp cl, '0' 
   je .skip_prefix_0		       
   dec edi			         
   jmp .put_each_num

.full0:
   mov cl,'0'			       ; 输入的数字为全0时，则只打印0
.put_each_num:
   push ecx			       
   call put_char
   add esp, 4
   inc edi			       
   mov cl, [put_int_buffer+edi]	       
   cmp edi,8
   jl .put_each_num
   popad
   ret

global set_cursor
set_cursor:
   pushad
   mov bx, [esp+36]
;;;;;;; 1 先设置高8位 ;;;;;;;;
   mov dx, 0x03d4			  ;索引寄存器
   mov al, 0x0e				  ;用于提供光标位置的高8位
   out dx, al
   mov dx, 0x03d5			  ;通过读写数据端口0x3d5来获得或设置光标位置 
   mov al, bh
   out dx, al

;;;;;;; 2 再设置低8位 ;;;;;;;;;
   mov dx, 0x03d4
   mov al, 0x0f
   out dx, al
   mov dx, 0x03d5 
   mov al, bl
   out dx, al
   popad
   ret


; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

extern disp_str
extern	disp_int

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_delay_milli_seconds	equ 1	;todo
_NT_print_str		equ	2	
_NT_P				equ	3
_NT_V				equ	4
INT_VECTOR_SYS_CALL equ 0x90

; 导出符号
global	get_ticks
global	delay_milli_seconds	;//todo
global	print_str
global	P
global	V


bits 32
[section .text]

; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	ret

;//todo
delay_milli_seconds:
	mov	ebx, [esp + 4]					;ebx保存传过来的时间参数
	mov	eax, _NR_delay_milli_seconds			;eax保存在系统调用函数表中的偏移量
	int INT_VECTOR_SYS_CALL				;调用系统调用
	ret

print_str:
	mov	ebx, [esp + 4]					;exb保存了指向字符串的指针
	mov	eax, _NT_print_str
	int INT_VECTOR_SYS_CALL
	ret									;关于我没写ret指令造成的惨剧

P:
	mov	ebx, [esp + 4]					;ebx保存了指向信号量的指针
	mov	eax, _NT_P
	int	INT_VECTOR_SYS_CALL
	ret

V:
	mov	ebx, [esp + 4]
	mov	eax, _NT_V
	int INT_VECTOR_SYS_CALL
	ret




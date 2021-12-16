
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "proc.h"

/* klib.asm */
PUBLIC void out_byte(u16 port, u8 value);
PUBLIC u8 in_byte(u16 port);
PUBLIC void disp_str(char *info);
PUBLIC void disp_color_str(char *info, int color);

/* protect.c */
PUBLIC void init_prot();
PUBLIC u32 seg2phys(u16 seg);

/* klib.c */
PUBLIC void delay(int time);
PUBLIC void clean__screen();

/* kernel.asm */
void restart();

/* main.c */
void ReadA();
void ReadB();
void ReadC();
void WriteD();
void WriteE();
void F();

/* i8259.c */
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);

/* clock.c */
PUBLIC void clock_handler(int irq);
PUBLIC void init_clock();

/* keyboard.c */
PUBLIC void init_keyboard();

/* 以下是系统调用相关 */

//todo
/* proc.c */
PUBLIC int sys_get_ticks(); /* sys_call */
PUBLIC void sys_delay_milli_seconds(int);
PUBLIC void sys_p(SEMAPHORE *s);
PUBLIC void sys_v(SEMAPHORE *s);

/* syscall.asm */
PUBLIC void sys_call(); /* int_handler */
PUBLIC int get_ticks();
PUBLIC void delay_milli_seconds(int);
PUBLIC void print_str(char *);
PUBLIC void P(SEMAPHORE *);
PUBLIC void V(SEMAPHORE *);


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "global.h"

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK *p_task = task_table;
	PROCESS *p_proc = proc_table;
	char *p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16 selector_ldt = SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++)
	{
		strcpy(p_proc->p_name, p_task->name); // name of the process
		p_proc->pid = i;					  // pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
			   sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
			   sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs = ((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ds = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.es = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.fs = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ss = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	/* proc_table[0].ticks = proc_table[0].priority = 1500;
	proc_table[1].ticks = proc_table[1].priority = 1500;
	proc_table[2].ticks = proc_table[2].priority = 1500; */

	//todo
	//初始时都可以被分配时间片
	for (int i = 0; i < NR_TASKS; i++)
	{
		proc_table[i].isWaiting = 0;
		proc_table[i].sleep_time = 0;
		proc_table[i].ticks = proc_table[2].priority = 10000;
	}

	//分配颜色
	proc_table[0].color = 0x09;
	proc_table[1].color = 0x02;
	proc_table[2].color = 0x04;
	proc_table[3].color = 0x06;
	proc_table[4].color = 0x0d;
	proc_table[5].color = 0x0e;

	k_reenter = 0;
	ticks = 0;

	mode = 0; //可调整
	readCount = 0;

	p_proc_ready = proc_table;

	init_clock();
	init_keyboard();

	clean_screen();

	restart(); //从ring0跳转到ring1，执行A进程

	while (1)
	{
	}
}

//disp_irq有问题，但我不知道到底有什么问题，好烦
void atomicP(SEMAPHORE *s)
{
	//disable_irq(CLOCK_IRQ);
	disable_int();
	/* disp_str(p_proc_ready->p_name);
	disp_str(" tyr to P\n"); */
	P(s);
	/* disp_str(p_proc_ready->p_name);
	disp_str(" success to P\n"); */
	enable_int();
	//enable_irq(CLOCK_IRQ);
}

void atomicV(SEMAPHORE *s)
{
	//disable_irq(CLOCK_IRQ);
	disable_int();
	/* disp_str(p_proc_ready->p_name);
	disp_str(" tyr to V\n"); */
	V(s);
	/* disp_str(p_proc_ready->p_name);
	disp_str(" success to V\n"); */
	enable_int();
	//enable_irq(CLOCK_IRQ);
}

void disp_read_start()
{
	disable_int();

	//打印读开始的信息
	//disp_int(rmutex.value);

	disp_color_str(p_proc_ready->p_name, p_proc_ready->color);
	disp_color_str(" ", p_proc_ready->color);
	disp_color_str("starts reading\n", p_proc_ready->color);
	/* disp_str(p_proc_ready->p_name);
	disp_str(" starts reading\n"); */
	enable_int();
}

disp_read_end()
{
	disable_int();
	//打印读结束信息
	disp_color_str(p_proc_ready->p_name, p_proc_ready->color);
	disp_color_str(" ", p_proc_ready->color);
	disp_color_str("ends reading\n", p_proc_ready->color);
	/* disp_str(p_proc_ready->p_name);
	disp_str(" ends reading\n"); */
	enable_int();
}

/*======================================================================*
                               read
 *======================================================================*/
void read(int slices)
{
	if (mode == 0)
	{ //读者优先

		atomicP(&rmutex);
		if (readCount == 0)
		{
			atomicP(&wmutex); //有进程在读的时候不让其它进程写
		}
		readCount++;
		atomicV(&rmutex);

		atomicP(&nr_readers);
		disp_read_start();
		//读操作消耗的时间片
		milli_delay(slices * TIMESLICE);
		disp_read_end();
		atomicV(&nr_readers);

		atomicP(&rmutex);
		readCount--;
		if (readCount == 0)
		{
			atomicV(&wmutex);
		}
		atomicV(&rmutex);
	}
}

/*======================================================================*
                               ReadA
 *======================================================================*/
void ReadA()
{
	while (1)
	{
		read(2);
	}
}

/*======================================================================*
                               ReadB
 *======================================================================*/
void ReadB()
{
	while (1)
	{
		read(3);
	}
}

/*======================================================================*
                               ReadC
 *======================================================================*/
void ReadC()
{
	while (1)
	{
		read(3);
	}
}

/*======================================================================*
                               WriteD
 *======================================================================*/
void WriteD()
{
	while (1)
	{
	}
}

/*======================================================================*
                               WriteE
 *======================================================================*/
void WriteE()
{
	while (1)
	{
	}
}

/*======================================================================*
                               F
 *======================================================================*/
void F()
{
	while (1)
	{
	}
}

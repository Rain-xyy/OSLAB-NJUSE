
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
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
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS *p;
	int greatest_ticks = 0;

	while (!greatest_ticks) //这个while循环没有什么意义，因为只能循环一次
	{
		for (p = proc_table; p < proc_table + NR_TASKS; p++) //todo
		{
			if (p->sleep_time > 0 || p->isWaiting == 1)
			{
				continue;
			}

			if (p->ticks > greatest_ticks)
			{
				greatest_ticks = p->ticks;
				p_proc_ready = p;
			}
		}

		if (!greatest_ticks) //所有进程的时间片都已经用完，重新按照优先级再分配一次时间片
		{
			for (p = proc_table; p < proc_table + NR_TASKS; p++)
			{
				p->ticks = p->priority;
			}
		}
	}

	/* disp_str(p_proc_ready->p_name);
	disp_str(" begins to work\n"); */
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

/*======================================================================*
                           sys_delay_milli_seconds
 *======================================================================*/
PUBLIC void sys_delay_milli_seconds(int milli_second) //todo
{
	p_proc_ready->sleep_time = milli_second; //睡眠时间以tick为单位，一个tick就是对应的一个ms
}

PUBLIC void sys_p(SEMAPHORE *s)
{
	s->value--;
	if (s->value < 0)
	{
		sleep(s);
	}
}

PUBLIC void sys_v(SEMAPHORE *s)
{
	s->value++;
	if (s->value <= 0)
	{
		wakeup(s);
	}
}

void sleep(SEMAPHORE *s)
{
	/* disp_color_str(p_proc_ready->p_name, p_proc_ready->color);
	disp_color_str(" sleeps in ", p_proc_ready->color);
	disp_str(s->name);
	disp_str("\n"); */

	p_proc_ready->isWaiting = 1;
	s->list[s->size] = p_proc_ready;
	s->size++;

	PROCESS temp = *p_proc_ready;

	//天坑，我吐了，这边sleep之后，会继续返回main中执行P操作后面的命令，因为当前进程的时间片没有消耗完，所以需要在手动切换进程
	//把陷入等待的进程移到进程队列的末尾，因为其刚刚陷入等待，需要等待的时间最长
	/* for (PROCESS *p = p_proc_ready; p < proc_table + NR_TASKS - 1; p++)
	{
		*p = *(p + 1);
	}
	proc_table[NR_TASKS - 1] = temp;

	for (PROCESS *p = proc_table; p < proc_table + NR_TASKS; p++)
	{
		disp_str(p->p_name);
	}
	disp_str("\n");
	schedule();
	disp_str(p_proc_ready->p_name);
	disp_str(" is chosen\n");
	restart(); */

	//disp_str(p_proc_ready->p_name);
	schedule();
	/* disp_str(" chooses ");
	disp_str(p_proc_ready->p_name);
	disp_str(" in ");
	disp_str(s->name);
	disp_str("\n"); */
	restart();
}

void wakeup(SEMAPHORE *s)
{
	if (s->size > 0)
	{
		/* disp_color_str(p_proc_ready->p_name, s->list[0]->color);
		disp_color_str(" wakes up ", s->list[0]->color);
		disp_color_str(s->list[0]->p_name, s->list[0]->color);
		disp_str(" in ");
		disp_str(s->name);
		disp_color_str("\n", s->list[0]->color); */

		s->list[0]->isWaiting = 0; //从当前信号量的等待队列中移出队首的等待进程
		for (int i = 0; i < s->size - 1; i++)
		{
			s->list[i] = s->list[i + 1];
		}
		s->size--;
	}
}

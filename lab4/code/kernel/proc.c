
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
	if (ticks - lastTicks >= 100)
	{
		lastTicks = ticks;
		p_proc_ready = proc_table + NR_TASKS - 1; //选中F进程
		isBlockedF = 0;
		return;
	}

	PROCESS *p;

	if (schedulable_queue_size == 0)
	{
		disp_str("no process is schedulable\n");
	}
	else
	{
		if (!numOfNotWorked())
		{
			reWork();
		}

		do
		{
			int process = schedulable_queue[0];
			p_proc_ready = proc_table + process;
			remove(0);	   //删除
			push(process); //移到队末
		} while (p_proc_ready->hasWorked == 1);

		/* int process = schedulable_queue[0];
		p_proc_ready = proc_table + process;
		remove(0);	   //删除
		push(process); //移到队末 */
	}
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
	int index = find();						 //把该进程移出可调度队列
	if (index != -1)
	{
		remove(index);
	}
	schedule();
	restart();
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
	//需要将当前的进程从可调度队列中移除
	int index = find(); //先找到当前进程在可调度队列中的下标，然后再删除当前进程
	if (index != -1)
	{
		remove(index); //todo
	}

	/* disp_str(p_proc_ready->p_name);
	disp_str(" sleeps in ");
	disp_str(s->name);
	disp_str("    "); */
	//disp_str("\n");

	p_proc_ready->isWaiting = 1;
	s->list[s->size] = p_proc_ready;
	s->size++;

	schedule();
	restart();

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
}

void wakeup(SEMAPHORE *s)
{
	if (s->size > 0)
	{
		/* disp_str(p_proc_ready->p_name);
		disp_str(" wakes up ");
		disp_str(s->list[0]->p_name);
		disp_str(" in ");
		disp_str(s->name);
		disp_str("    "); */
		//disp_str("\n");

		push(s->list[0]->pid); //移入可调度队列

		s->list[0]->isWaiting = 0; //从当前信号量的等待队列中移出队首的等待进程
		for (int i = 0; i < s->size - 1; i++)
		{
			s->list[i] = s->list[i + 1];
		}
		s->size--;
	}
}

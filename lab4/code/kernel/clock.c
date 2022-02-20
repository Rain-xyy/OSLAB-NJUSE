
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               clock.c
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
                           clock_handler
 *======================================================================*/
PUBLIC void clock_handler(int irq)
{
        ticks++;
        /* p_proc_ready->ticks--; */

        if (k_reenter != 0)
        {
                return;
        }

        for (int i = 0; i < NR_TASKS; i++)
        {
                if (proc_table[i].sleep_time > 0)
                {
                        proc_table[i].sleep_time--;
                        if (proc_table[i].sleep_time == 0)
                        {
                                //移入可调度队列
                                push(proc_table[i].pid);
                        }
                }
        }

        schedule();
}

/*======================================================================*
                              milli_delay
 *======================================================================*/
PUBLIC void milli_delay(int milli_sec)
{
        int t = get_ticks(); //此函数是线程共享的，比如A线程和B线程都需要延迟20ms，然后A线程和B线程轮流被分配时间片，事实上在A的时间片里面B线程的延迟也在被及时，因为ticks对于每个线程是共享变化的

        while (((get_ticks() - t) * 1000 / HZ) < milli_sec) //todo
        {
        }
}

/*======================================================================*
                           init_clock
 *======================================================================*/
PUBLIC void init_clock()
{
        /* 初始化 8253 PIT */
        out_byte(TIMER_MODE, RATE_GENERATOR);
        out_byte(TIMER0, (u8)(TIMER_FREQ / HZ));
        out_byte(TIMER0, (u8)((TIMER_FREQ / HZ) >> 8));

        put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
        enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */
}

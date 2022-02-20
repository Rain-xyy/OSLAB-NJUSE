
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "global.h"

PUBLIC PROCESS proc_table[NR_TASKS];

PUBLIC char task_stack[STACK_SIZE_TOTAL];

PUBLIC TASK task_table[NR_TASKS] = {{ReadA, STACK_SIZE_ReadA, "ReadA"},
                                    {ReadB, STACK_SIZE_ReadB, "ReadB"},
                                    {ReadC, STACK_SIZE_ReadC, "ReadC"},
                                    {WriteD, STACK_SIZE_WriteD, "WriteD"},
                                    {WriteE, STACK_SIZE_WriteE, "WriteE"},
                                    {F, STACK_SZIE_F, "F"}}; //用户进程

PUBLIC irq_handler irq_table[NR_IRQ]; //中断处理函数，其中0位置为始终中断处理

//todo
PUBLIC system_call sys_call_table[NR_SYS_CALL] = {sys_get_ticks, sys_delay_milli_seconds, disp_str, sys_p, sys_v}; //系统调用的函数

SEMAPHORE rmutex = {1, 0, "rmutex"};         //读信号量，控制对于readCount进行互斥访问
SEMAPHORE wmutex = {1, 0, "wmutex"};         //写信号量，控制对于writeCount的互斥访问
SEMAPHORE rw_mutex = {1, 0, "rw_mutex"};     //读写信号量，是读写互斥锁，用于在读者优先中实现读写互斥
SEMAPHORE nr_readers = {1, 0, "nr_readers"}; //读上限人数信号量，控制读上限人数
SEMAPHORE r = {1, 0, "r"};                   //写者优先中让写进程开始的时候申请阻塞读进程
SEMAPHORE w = {1, 0, "w"};                   //在写者优先中，允许至多一个写进程在写
SEMAPHORE queue = {1, 0, "queue"};           //在写进程优先中，让多的信号量都在queue上排队，避免在r信号量上排长队使得写进程等待很久

int schedulable_queue[NR_TASKS - 1] = {0, 1, 2, 3, 4}; //减去一个进程是因为F进程特殊判断来调用
int schedulable_queue_size = NR_TASKS - 1;

void push(int n) //添加新的就绪进程
{
    schedulable_queue[schedulable_queue_size] = n;
    schedulable_queue_size++;
}

void remove(int n) //移出就绪进程
{
    for (int i = n; i < schedulable_queue_size - 1; i++)
    {
        schedulable_queue[i] = schedulable_queue[i + 1];
    }
    schedulable_queue_size--;
}

int find()
{
    for (int i = 0; i < schedulable_queue_size; i++)
    {
        if (schedulable_queue[i] == p_proc_ready->pid)
        {
            return i;
        }
    }
    return -1;
}

int numOfNotWorked()
{
    int notWorked = 0;
    for (int i = 0; i < schedulable_queue_size; i++)
    {
        if (proc_table[schedulable_queue[i]].hasWorked == 0)
        {
            notWorked++;
        }
    }
    return notWorked; //返回的是没有工作过的进程的数量
}

void reWork()
{
    for (int i = 0; i < schedulable_queue_size; i++)
    {
        proc_table[schedulable_queue[i]].hasWorked = 0;
    }
}

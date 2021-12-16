
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

PUBLIC PROCESS *sleep_table[MAX_SLEEP_PROCESS];
int sleep_size = 0;

SEMAPHORE rmutex = {1, 0, "rmutex"};         //读信号量
SEMAPHORE wmutex = {1, 0, "wmutex"};         //写信号量
SEMAPHORE nr_readers = {2, 0, "nr_readers"}; //读上限人数
SEMAPHORE S;                                 //互斥信号量
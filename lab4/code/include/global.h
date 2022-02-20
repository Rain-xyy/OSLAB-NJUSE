
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* EXTERN is defined as extern except in global.c */
#ifdef GLOBAL_VARIABLES_HERE
#undef EXTERN
#define EXTERN
#endif

EXTERN int ticks;
EXTERN int lastTicks;

EXTERN int mode;
EXTERN int readCount;
EXTERN int writeCount;
EXTERN int isBlockedF;

EXTERN int disp_pos;
EXTERN u8 gdt_ptr[6]; // 0~15:Limit  16~47:Base
EXTERN DESCRIPTOR gdt[GDT_SIZE];
EXTERN u8 idt_ptr[6]; // 0~15:Limit  16~47:Base
EXTERN GATE idt[IDT_SIZE];

EXTERN u32 k_reenter;

EXTERN TSS tss;
EXTERN PROCESS *p_proc_ready;

extern PROCESS proc_table[];
extern char task_stack[];
extern TASK task_table[];
extern irq_handler irq_table[];

extern PROCESS *sleep_table[];
extern int sleep_size;

extern SEMAPHORE rmutex;
extern SEMAPHORE wmutex;
extern SEMAPHORE rw_mutex;
extern SEMAPHORE nr_readers;
extern SEMAPHORE r;
extern SEMAPHORE w;
extern SEMAPHORE queue;

extern SEMAPHORE specialMutex;

extern int schedulable_queue[];
extern int schedulable_queue_size;

void push(int);
void remove(int);
int find();
int numOfNotWorked();
void reWork();
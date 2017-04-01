#define GLOBAL_VARIABLES_HERE
/*
#include "type.h"
#include "const.h"
#include "protect.h"
#include "console.h"
#include "tty.h"
#include "proc.h"
#include "proto.h"
#include "fs.h"
#include "global.h"*/
#include "head.h"
PUBLIC  PROCESS*		p_proc_ready;
PUBLIC	PROCESS			proc_table[NR_TASKS + NR_PROCS];
/*pointer stackzie name priority*/
PUBLIC	TASK 			task_table[NR_TASKS]={
							{task_tty, STACK_SIZE_TTY, "tty",1},
							{task_sys, STACK_SIZE_SYS, "sys",9},
							{task_hd, STACK_SIZE_HD, "hd",9},
							//{task_fs, STACK_SIZE_FS, "fs",30},
						
						};

PUBLIC	TASK 			user_proc_table[NR_PROCS]={
							{TestA,STACK_SIZE_TESTA,"TestA",4},
							{TestB,STACK_SIZE_TESTB,"TestB",3},
							{TestC,STACK_SIZE_TESTC,"TestC",2},
							//	{TestD,STACK_SIZE_TESTC,"TestD",5}
							};
/*和proc_table(task_table)中的进程序号对应*/				
struct dev_drv_map dd_map[] ={
	/*driver nr 		major device nr*/
	{INVALID_DRIVER},   /* 0 : Unused */
	{INVALID_DRIVER},	/* 1 : reserved for floppy driver */
	{INVALID_DRIVER},	/* 2 : reserved for cdrom driver */
	{TASK_HD},			/* 3 : Hard disk */
	{TASK_TTY},			/* 4 : TTY*/
	{INVALID_DRIVER},	/* 5 : reserved for scsi driver */
};



PUBLIC TTY 				tty_table[NR_CONSOLES];
PUBLIC CONSOLE 			console_table[NR_CONSOLES];
PUBLIC TTY* 			p_tty;

PUBLIC irq_handler 		irq_table[NR_IRQ]; /*system interupt*/

PUBLIC system_call 		sys_call_table[NR_SYS_CALL] = 
							{sys_printx,sys_sendrec};/*,*/

PUBLIC	char			task_stack[STACK_SIZE_TOTAL];

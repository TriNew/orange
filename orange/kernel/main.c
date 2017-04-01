#include "head.h"

PUBLIC int kernel_main()
{

	
	disp_str("-------kernel_main begins--------------\n");

	TASK * p_task 		 = task_table;
	PROCESS *p_proc 	 = proc_table;
	char* p_task_stack   = task_stack+STACK_SIZE_TOTAL;/*进程空间的栈基址加大小*/
	u16 	selector_ldt = SELECTOR_LDT_FIRST;  /*0x28, 5th in GDT*/
	int 	iTask;
	u8 		privilege;
	u8 		rpl;
	int 	eflags;

	for(iTask=0 ; iTask<NR_TASKS + NR_PROCS; iTask++)
	{
		if(iTask < NR_TASKS)
		{
			p_task 	  = (TASK *)task_table + iTask;
			privilege = PRIVILEGE_TASK;
			rpl 	  = RPL_TASK;
			eflags 	  = 0x1202;  /*IF=1,IOPL=1 bit 2 is always 1*/
		}
		else 
		{
			p_task 	  =(TASK *) user_proc_table + iTask - NR_TASKS;
			privilege = PRIVILEGE_USER;
			rpl 	  = RPL_USER;
			eflags 	  = 0x202; /*IF=1, bit 2 is always 1*/

			
		}
		strcpy(p_proc->p_name,p_task->name);
		p_proc->pid = iTask ;
		p_proc->ldt_sel = selector_ldt;

		p_proc->ticks = p_task->priority;
		p_proc->priority = p_task->priority;

		memcpy(&p_proc->ldts[0],&gdt[SELECTOR_KERNEL_CS>>3],sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 =DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1],&gdt[SELECTOR_KERNEL_DS>>3],sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 =DA_DRW | privilege << 5;
/*idt0是SELECTOR_KERNEL_CS那个        SA_TIL设置使用LDT   RPL_TASK设置位ring1*/
		p_proc->regs.cs = ((8*0) & SA_RPL_MASK & SA_TI_MASK)|SA_TIL | rpl;
		p_proc->regs.ds = ((8*1) & SA_RPL_MASK & SA_TI_MASK)|SA_TIL | rpl;
		p_proc->regs.es = ((8*1) & SA_RPL_MASK & SA_TI_MASK)|SA_TIL | rpl;
		p_proc->regs.fs = ((8*1) & SA_RPL_MASK & SA_TI_MASK)|SA_TIL | rpl;
		p_proc->regs.ss = ((8*1) & SA_RPL_MASK & SA_TI_MASK)|SA_TIL | rpl;
		p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK)| rpl;

		p_proc->regs.eip= (u32)p_task->initial_eip;
/*进入进程后指向栈顶 0x8000*/
		p_proc->regs.esp= (u32)p_task_stack;
		p_proc->regs.eflags= eflags;
		p_proc->nr_tty = 0;	

		p_proc->p_recvfrom = NO_TASK;
		p_proc->p_sendto =NO_TASK;
		p_proc->has_int_msg = 0;
		p_proc->q_sending =0;
		p_proc->next_sending =0;
		p_proc->p_flags = 0;
		p_proc->p_msg = 0;

		p_task_stack -= p_task->stacksize;

		p_proc++;
		p_task++;
		selector_ldt += 1<<3;
	
		
	}

	k_reenter  = 0;
	ticks = 0;
	p_proc_ready = proc_table;
/*if(p_proc_ready->regs.cs == 8)
{

	disp_int(p_proc_ready->ticks);
	disp_str(".");
	disp_int(p_proc_ready->priority);
	disp_str(".");
	disp_str(p_proc_ready->p_name);
	while(1);
}*/
/*初始化8253定时器*/
	
	init_clock();
	init_keyboard();
	restart();     /*enter process*/
	while(1){
		//keyboard_read();
	}


}
/*get_ticks by msg
*BOTH表示将一个消息发送给目的进程，并等待目的进程将消息处理后发送给原进程
*/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH,TASK_SYS, &msg);

	return msg.RETVAL;
}
/*panic is used only in ring0 or ring1*/
PUBLIC void panic(const char *fmt,...)
{
	int i ;
	char buf[256];
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf,fmt,arg);
	print1("%c !!panic!! %s",MAG_CH_PANIC,buf);

	/*should never arrive here*/
	__asm__ __volatile__("ud2");
}
void TestA()
{
	while(1)
	{
	  // printf("A. ");

		//printf("%d.",get_ticks());
		//milli_delay(1000);
	}
}
void TestB()
{
	
	while(1)
	{
	  //	printf("B. ");
		//milli_delay(10);
	}
}
void TestC()
{
	
	while(1)
	{
	  //		printf("C. ");
		//disp_str("C.");
		//milli_delay(10);
	}
}
/*void TestD()
{
	
	while(1)
	{
	  //	printf("D. ");
		//disp_str("C.");
		//milli_delay(10);
	}
}
*/

#include "head.h"

PUBLIC void schedule()
{
	PROCESS *p;
	int 	greatest_ticks = 0;
	//if(p_proc_ready->ticks<=0)
	//	p_proc_ready->ticks = p_proc_ready->priority;

	while(!greatest_ticks)
	{

		for(p =&FIRST_PROC; p <= &LAST_PROC;p++)
		{
			if(p->p_flags == 0)
			{
				if(p->ticks > greatest_ticks)
					{
						greatest_ticks = p->ticks ;
						p_proc_ready = p;
					}
			}
		}
			
		if(!greatest_ticks)
		{
			for(p =&FIRST_PROC; p <= &LAST_PROC;p++)
				if(p->p_flags == 0)
				{
					p->ticks = p->priority;
				}
		}
	}


	//disp_str(p_proc_ready->p_name);
	//						disp_int(p_proc_ready->ticks);disp_str(" ");
}
/********************************************************************
**********************************************************************/
/*send_rec
*<ring1~3> IPC syscall
* It is an encapsulation of sendrec,invoking 'sendrec' directly should be
*avoided
*/
PUBLIC int send_recv(int function , int src_dest, MESSAGE* msg)
{
	int ret = 0;

	if(function == RECEIVE)
		memset(msg, 0 ,sizeof(MESSAGE));

	switch(function)
	{
		case BOTH:
			ret = sendrec(SEND, src_dest, msg);
			if(ret == 0)
				ret = sendrec(RECEIVE, src_dest, msg);
			break;
		case SEND:
		case RECEIVE:
			ret = sendrec(function, src_dest, msg);
			break;
		default:
			assert((function == BOTH) ||
				(function == SEND) || (function == RECEIVE));
			break;
	}
	return ret;
}




/***************************************************************** 							va2la
*<ring0-1> Virtual addr ---> Linear addr 
* return the linear address for the given virtual address
*****************************************************************/
PUBLIC void * va2la(int pid, void *va)
{
	PROCESS *p = &proc_table[pid];

	u32 seg_base = ldt_seg_linear(p,INDEX_LDT_RW);
	u32 la =seg_base + (u32)va;

	if(pid < NR_TASKS + NR_PROCS)
		assert(la == (u32)va);

	return (void*)la;
}



/*<ring0 >This routine is called after 'p_flags has been set(!=0)
it calls 'schedule()' to choose another proc as the 'proc_ready'
p is the proc to be blocked
*/
PRIVATE void block(PROCESS *p)
{
	assert(p->p_flags);
	schedule();
}
/*<ring0>This is a dummy routine. It does nothing actually. When it is 
called. The 'p_flags' should have been cleared(==0)
p is the unblocked proc*/
PRIVATE void unblock(struct proc* p)
{
	assert(p->p_flags == 0);
}
/*<ring0> deadlock
return Zero if success
*/
PRIVATE int deadlock(int src, int dest)
{
	PROCESS *p = proc_table + dest;
	while(1)
	{
		if(p->p_flags & SENDING)/*若目标不在发送状态，不死锁*/
		{
			if(p->p_sendto == src)/*打印死锁进程名*/
			{
				p = proc_table + dest;
				print1("=_=%s",p->p_name);
				do{
					assert(p->p_msg);
					p = proc_table + p->p_sendto;
					print1("->%s",p->p_name);
				}while(p != proc_table + src);
				print1("=_=!!!");

				return 1;
			}
			p =proc_table + p->p_sendto;
		}
		else {break;}
	}
	return 0;
}


/*
<ring0> Send a message to the dest proc.If dest is blocked waiting for
the message,copy the message to it and unblock dest Otherwise the caller
will be blocked and append to the dest's sending queue
return zero if success
*/
PRIVATE int msg_send(PROCESS *current,int dest, MESSAGE *m)
{
	PROCESS *sender = current;
	struct proc* p_dest = proc_table + dest;
	/*not send to self*/
	assert(proc2pid(sender) != dest);

	if(deadlock(proc2pid(sender) , dest))
	{
		panic(">>DEADLOCK<< %s-->%s",sender->p_name,p_dest->p_name);
	}

	if((p_dest->p_flags & RECEIVING) &&
			(p_dest->p_recvfrom == proc2pid(sender) ||
			p_dest->p_recvfrom ==ANY))
	{
		assert(p_dest->p_msg);
		assert(m);

		phys_copy(va2la(dest, p_dest->p_msg),/*va2la(pid,vaddr)*/
					va2la(proc2pid(sender),m),
					sizeof(MESSAGE));
		p_dest->p_msg 		= 0;
		p_dest->p_flags    &= ~RECEIVING;/*dest has received the msg*/
		p_dest->p_recvfrom 	= NO_TASK;

		unblock(p_dest);/*unblock proc dest*/

		assert(p_dest->p_flags		== 0);
		assert(p_dest->p_msg		== 0);
		assert(p_dest->p_recvfrom 	== NO_TASK);
		assert(p_dest->p_sendto 	== NO_TASK);
		assert(sender->p_flags 		== 0);
		assert(sender->p_msg 		== 0);
		assert(sender->p_recvfrom 	== NO_TASK);
		assert(sender->p_sendto 	== NO_TASK);
	}
	else 
	{ /*目的进程没准好接收发送进程的消息*/
		sender->p_flags |= SENDING;
		assert(sender->p_flags == SENDING);
		sender->p_sendto =dest;
		sender->p_msg 	 = m;

		struct proc * p;/*插入dest队列末*/
		if(p_dest->q_sending)
		{
			p = p_dest->q_sending; 
			while(p->next_sending)
				p = p->next_sending;

			p->next_sending = sender;
		}
		else
		{
			p_dest->q_sending = sender;
		}
		sender->next_sending = 0;

		block(sender);

		assert(sender->p_flags 		== SENDING);
		assert(sender->p_msg 		!= 0);
		assert(sender->p_recvfrom 	==NO_TASK);
		assert(sender->p_sendto 	== dest);
	}
	return 0;
}
/*
<ring0> Try to get a message from the src proc If src is blocked 
sending the message ,copy the message from it and unblock src,Otherwise
the caller will be blocked 

return zero if success 
*/
PRIVATE int msg_receive(PROCESS *current,int src, MESSAGE *m)
{
	PROCESS * p_who_wanna_recv = current;/*= p_ready_proc*/
	PROCESS * p_from = 0; /*from which the message will be fetched */
	PROCESS * prev   = 0;
	
	int copyok =0 ;
	/*do not receive self msg*/
	assert(proc2pid(p_who_wanna_recv) != src);
/*There is an interrupt needs p_who_wanna_recv's handing and
*p_who_wanna_recv is ready to handle it
*/
	if((p_who_wanna_recv->has_int_msg) && 
		((src == ANY) || (src == INTERRUPT)))
	{
		MESSAGE msg;
		reset_msg(&msg);
		msg.source = INTERRUPT;
		msg.type 	= HARD_INT;
		
		assert(m);
		phys_copy(va2la(proc2pid(p_who_wanna_recv), m), &msg, 
					sizeof(MESSAGE));

		p_who_wanna_recv->has_int_msg = 0;

		assert(p_who_wanna_recv->p_flags == 0);
		assert(p_who_wanna_recv->p_msg == 0);
		assert(p_who_wanna_recv->p_sendto == NO_TASK);
		assert(p_who_wanna_recv->has_int_msg == 0);

		return 0;
	}
/*p_who_wanna_recv is ready to receive message from ANY proc
* we will check the sending queue and pick the first proc in it 
* receive int,ANY ,src msg. if no proc sending msg set p_flags and 
* blocked  
*/
	if(src == ANY)
	{
		if(p_who_wanna_recv->q_sending)
		{
			p_from = p_who_wanna_recv->q_sending;
			copyok = 1;

			assert(p_who_wanna_recv->p_flags == 0);
			assert(p_who_wanna_recv->p_msg == 0);
			assert(p_who_wanna_recv->p_recvfrom == NO_TASK);
			assert(p_who_wanna_recv->p_sendto == NO_TASK);
			assert(p_who_wanna_recv->q_sending != 0);
			assert(p_who_wanna_recv->p_flags == 0);


			assert(p_from->p_flags == SENDING);
			assert(p_from->p_msg != 0);
			assert(p_from->p_recvfrom == NO_TASK);
			assert(p_from->p_sendto == proc2pid(p_who_wanna_recv));

		}

	}
	else /*receive a message from a certain proc: src or interrupt*/
	{/*PROCESS proc_table[NR_TASKS + NR_PROCS];*/
		p_from = &proc_table[src];

		if((p_from->p_flags & SENDING) &&
			(p_from->p_sendto == proc2pid(p_who_wanna_recv)))
		{/*src is sending a message to p_who_wanna_recv*/
			copyok = 1;
		/*p_from must have been appended to the queue
		*so the queue must not be NULL
		*/
			PROCESS * p = p_who_wanna_recv->q_sending;
			assert(p);
			/*查找接收队列，找到发送进程pid=src的进程*/
			while(p)
			{
				assert(p_from->p_flags & SENDING);
				if(proc2pid(p) == src)
				{
					p_from = p;
					break;
				}
				prev = p;
				p = p->next_sending;
			}

			assert(p_who_wanna_recv->p_flags == 0);
			assert(p_who_wanna_recv->p_msg == 0);
			assert(p_who_wanna_recv->p_recvfrom == NO_TASK);
			assert(p_who_wanna_recv->p_sendto == NO_TASK);

			assert(p_from->p_flags == SENDING);
			assert(p_from->p_msg != 0);
			assert(p_from->p_recvfrom == NO_TASK);
			assert(p_from->p_sendto == proc2pid(p_who_wanna_recv));

		}
	}

	if(copyok)
	{   /*first in the queue*/
		if(p_from == p_who_wanna_recv->q_sending)
		{
			assert(prev == 0);
			p_who_wanna_recv->q_sending = p_from->next_sending;
			p_from->next_sending = 0;

		}
		else
		{
			assert(prev);
			prev->next_sending = p_from->next_sending;
			p_from->next_sending = 0;
		}
		assert(m);
		assert(p_from->p_msg);

		phys_copy(va2la(proc2pid(p_who_wanna_recv), m),
				va2la(proc2pid(p_from), p_from->p_msg),
				sizeof(MESSAGE));
		p_from->p_msg = 0;
		p_from->p_sendto = NO_TASK;
		p_from->p_flags &= ~SENDING;
		unblock(p_from);
	}
	else
	{/*no proc is sending messages
	  * set p_flags so that will not be schedulded until unblocked
	  */
		p_who_wanna_recv->p_flags |= RECEIVING;

		p_who_wanna_recv->p_msg    = m;
/*发送进程是ANY（发给所有进程） 接收进程是ANY，否则是发送进程pid*/
		if(src == ANY)
			p_who_wanna_recv->p_recvfrom = ANY;
		else if(src == INTERRUPT)
			p_who_wanna_recv->p_recvfrom = INTERRUPT;
		else
			p_who_wanna_recv->p_recvfrom = proc2pid(p_from);

		block(p_who_wanna_recv);

//printf("%s",p_who_wanna_recv->p_name);
//printf("%x\n",src);

		assert(p_who_wanna_recv->p_flags == RECEIVING);
		assert(p_who_wanna_recv->p_msg != 0);
		assert(p_who_wanna_recv->p_recvfrom != NO_TASK);
		assert(p_who_wanna_recv->p_sendto == NO_TASK);
		assert(p_who_wanna_recv->has_int_msg == 0);

	}
	return 0;
}


/*ring 0  The core routine of system call sendrec()
*return zero if success
*function : SEND or RECEIVE
*src_dest : the proc to receive or send msg
*p : the proc which is running and it is it sendding or receiving the msg */
PUBLIC int sys_sendrec(int function,
						int src_dest,
						MESSAGE* m ,
						struct proc* p)
{

/*
* ring0初始化后k_reenter=0(初始值),进入ring1-3后k_reenter>0，返回ring0后k_reenter=0
*/
	assert(k_reenter == 0 );
	assert((src_dest >=0 && src_dest <= NR_TASKS + NR_PROCS) 
			|| src_dest == ANY || src_dest == INTERRUPT);
	int 	ret = 0;
	int 	caller = proc2pid(p);
	MESSAGE* mla = (MESSAGE*)va2la(caller,m);
	mla->source = caller;
/*消息的源地址不等于目的地址*/
	assert(mla->source != src_dest);

	if(function == SEND)
	{
		ret = msg_send(p, src_dest, m);
		if(ret != 0)
			return ret ;
	}
	else if(function == RECEIVE)
	{
		ret = msg_receive(p, src_dest, m);
		if(ret != 0)
			return ret;
	}
	else 
	{
		panic("{sys_sendrec} invalid function: "
				"%d (SEND:%d, RECEIVE:%d).",function, SEND, RECEIVE);
	}
	return 0;
}

/********************************************************
 * 					ldt_seg_linear
 * get the linear addr of a certain segment of a given proc<ring0-1>
 **************************************************************/
 PUBLIC int ldt_seg_linear(PROCESS *p ,int idx)
 {
 	 DESCRIPTOR *d = &(p->ldts[idx]);
 	return d->base_high << 24 | d->base_mid << 16 | d->base_low;
 }


/*******************************************************************
*						 reset_msg
* <ring 0-3> Clear up a MESSAGE by setting each byte to 0
*******************************************************************/
PUBLIC void reset_msg(MESSAGE* p)
{
	memset(p, 0,sizeof(MESSAGE));
}

int proc2pid(PROCESS *p)
{
	return p->pid;
}

/*****************************************************************************
 *                                inform_int
 *****************************************************************************/
/**
 * <Ring 0> Inform a proc that an interrupt has occured.
 * 
 * @param task_nr  The task which will be informed.
 *****************************************************************************/
PUBLIC void inform_int(int task_nr)
{
	struct proc* p = proc_table + task_nr;

	if ((p->p_flags & RECEIVING) && /* dest is waiting for the msg */
	    ((p->p_recvfrom == INTERRUPT) || (p->p_recvfrom == ANY))) {
		p->p_msg->source = INTERRUPT;
		p->p_msg->type = HARD_INT;
		p->p_msg = 0;
		p->has_int_msg = 0;
		p->p_flags &= ~RECEIVING; /* dest has received the msg */
		p->p_recvfrom = NO_TASK;
		assert(p->p_flags == 0);
		unblock(p);
//printf("inform,%d",p->p_flags);
		assert(p->p_flags == 0);
		assert(p->p_msg == 0);
		assert(p->p_recvfrom == NO_TASK);
		assert(p->p_sendto == NO_TASK);
	}
	else {
		p->has_int_msg = 1;
	}

}

/*****************************************************************************
 *                                dump_proc
 *****************************************************************************/
PUBLIC void dump_proc(struct proc* p)
{
	char info[STR_DEFAULT_LEN];
	int i;
	int text_color = 0x74 ;
	//int text_color = MAKE_COLOR(GREEN, RED);

	int dump_len = sizeof(struct proc);
	/*set display pos in the video memory*/
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, 0);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, 0);

	sprintf(info, "byte dump of proc_table[%d]:\n", p - proc_table); disp_color_str(info, text_color);
	for (i = 0; i < dump_len; i++) {
		sprintf(info, "%x.", ((unsigned char *)p)[i]);
		disp_color_str(info, text_color);
	}

	/* printl("^^"); */

	disp_color_str("\n\n", text_color);
	sprintf(info, "ANY: 0x%x.\n", ANY); disp_color_str(info, text_color);
	sprintf(info, "NO_TASK: 0x%x.\n", NO_TASK); disp_color_str(info, text_color);
	disp_color_str("\n", text_color);

	sprintf(info, "ldt_sel: 0x%x.  ", p->ldt_sel); disp_color_str(info, text_color);
	sprintf(info, "ticks: 0x%x.  ", p->ticks); disp_color_str(info, text_color);
	sprintf(info, "priority: 0x%x.  ", p->priority); disp_color_str(info, text_color);
	sprintf(info, "pid: 0x%x.  ", p->pid); disp_color_str(info, text_color);
	sprintf(info, "name: %s.  ", p->p_name); disp_color_str(info, text_color);
	disp_color_str("\n", text_color);
	sprintf(info, "p_flags: 0x%x.  ", p->p_flags); disp_color_str(info, text_color);
	sprintf(info, "p_recvfrom: 0x%x.  ", p->p_recvfrom); disp_color_str(info, text_color);
	sprintf(info, "p_sendto: 0x%x.  ", p->p_sendto); disp_color_str(info, text_color);
	sprintf(info, "nr_tty: 0x%x.  ", p->nr_tty); disp_color_str(info, text_color);
	disp_color_str("\n", text_color);
	sprintf(info, "has_int_msg: 0x%x.  ", p->has_int_msg); disp_color_str(info, text_color);
}


/*****************************************************************************
 *                                dump_msg
 *****************************************************************************/
PUBLIC void dump_msg(const char * title, MESSAGE* m)
{
	int packed = 0;
	printl("{%s}<0x%x>{%ssrc:%s(%d),%stype:%d,%s(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)%s}%s",  //, (0x%x, 0x%x, 0x%x)}",
	       title,
	       (int)m,
	       packed ? "" : "\n        ",
	       proc_table[m->source].p_name,
	       m->source,
	       packed ? " " : "\n        ",
	       m->type,
	       packed ? " " : "\n        ",
	       m->u.m3.m3i1,
	       m->u.m3.m3i2,
	       m->u.m3.m3i3,
	       m->u.m3.m3i4,
	       (int)m->u.m3.m3p1,
	       (int)m->u.m3.m3p2,
	       packed ? "" : "\n",
	       packed ? "" : "\n"/* , */
		);
}

#include "head.h"


#define TTY_FIRST 	(tty_table)
#define TTY_END 	(tty_table + NR_CONSOLES)

/*
typedef struct s_tty
{
	u32 	in_buf[TTY_IN_BYTES];
	u32* 	p_inbuf_head;
	u32* 	p_inbuf_tail;
	int 	inbuf_count;
	struct 	s_console * p_console;
}TTY;
*/

PRIVATE void put_key(TTY* p_tty, u32 key)
{
	if(p_tty->inbuf_count < TTY_IN_BYTES)
			{
				*(p_tty->p_inbuf_head) = key;
				p_tty->p_inbuf_head++;
				if(p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES)
				{
					p_tty->p_inbuf_head = p_tty->in_buf;
				}
				p_tty->inbuf_count++;
			}
}
PRIVATE void tty_do_read(TTY* p_tty)
{
	if(is_current_console(p_tty->p_console))
	{
		keyboard_read(p_tty);
	}
}
PRIVATE void tty_do_write(TTY* p_tty)
{
	if(p_tty->inbuf_count)
	{
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail++;
		if(p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES)
		{
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;
		out_char(p_tty->p_console,ch);
	}

}
/*=========================================================*/
PUBLIC void task_tty()
{
	
	for(p_tty = TTY_FIRST;p_tty<TTY_END;p_tty++)
	{
		init_tty(p_tty);
	}

	select_console(0);
	
	while(1)
	{
		for(p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++)
		{

			tty_do_read(p_tty);  /*call keyboard_read*/
			tty_do_write(p_tty);

		}
	}

}
PUBLIC void in_process(TTY* p_tty,u32 key)
{
	char output[2] = {'\0','\0'};
/*功能键的键值从0x101（ESC）开始,可打印键的键值都小于0x100*/
	/*FLAG_EXT:0x100*/
	if(!(key & FLAG_EXT))
		{
			put_key(p_tty, key);/*add key to p_tty->p_inbuf_head*/
		}
	else{
			int raw_code = key & MASK_RAW;  /*MASK_RAW:0x01FF*/
			switch(raw_code)
				{
					case UP:
						if((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
						{
							scroll_screen(p_tty->p_console, SCREEN_SIZE, SCR_UP);
						}
						else
						{scroll_screen(p_tty->p_console, SCREEN_WIDTH, SCR_UP);}
					
						break;
					case DOWN:
						if((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
						{
							scroll_screen(p_tty->p_console, SCREEN_SIZE, SCR_DN);
						}
						else{
							scroll_screen(p_tty->p_console, SCREEN_WIDTH, SCR_DN);
						}
						break;
					case ENTER:
						put_key(p_tty, '\n');
						break;
					case BACKSPACE:
						put_key(p_tty, '\b');
						break;
					case F1:
					case F2:
					case F3:
					case F4:
					case F5:
					case F6:
					case F7:
							if((key & FLAG_CTRL_L)||(key & FLAG_CTRL_R))
							{
								select_console(raw_code - F1);
							}

						break;
					default :
					break;

				}
				
		}
	disable_int();
	out_byte(CRTC_ADDR_REG , CURSOR_H);
	out_byte(CRTC_DATA_REG , (((disp_pos)/2)>>8)&0xFF);
	out_byte(CRTC_ADDR_REG , CURSOR_L);
	out_byte(CRTC_DATA_REG , ((disp_pos/2))&0xFF);
	enable_int();
}
PUBLIC void init_tty(TTY* p_tty)
{
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;
	init_screen(p_tty);
}
/*
PUBLIC void tty_write(TTY* p_tty, char * buf ,int len )
{	
	char * p = buf ;
	int i = len;
	while(i)
	{
		out_char(p_tty->p_console, *p++);
		i--;
	}
}
PUBLIC int sys_write(char* buf, int len,int unused, PROCESS* p_proc)
{
	//disp_str("tty");
	tty_write(&tty_table[p_proc->nr_tty], buf, len);
	return 0;
}
*/
PUBLIC int sys_printx(int _unused1,int _unused2,char* s,PROCESS* p_proc)
{
	/*s is the pointer of buf */
	const char *p;
	char ch;
	
	char reenter_err[] = "? k_reenter is incorrect for unknown reason";
	reenter_err[0] = MAG_CH_PANIC;
	/*
		note code in both Ring0 hand ring1-3 may invoke printx();
		-# if this happens in ring 0 ,no linear-physical address mapping 
			is need 
			k_reenter > 0 when code in ring 0 calls printx()
			an 'interrupt re-enter' will occur ,Thus 'k_reenter' will be increased
			by 'kernel.asm::save' and be greater than 0
		-# if this happens in ring1-3 k_reenter == 0  
	*/
	if(k_reenter == 0)
		p = va2la(proc2pid(p_proc),s);
	else if (k_reenter > 0)
		p = s;
	else 
		p = reenter_err;
	/*
	if assertion fails in any TASK the system will be halted 
	if it fails in a USER PROC ,it'll return any normal syscall does 
	*/
	if((*p == MAG_CH_PANIC) || 
		(*p == MAG_CH_ASSERT && p_proc_ready < &proc_table[NR_TASKS]))
	{
		disable_int();
		char *v = (char*)V_MEM_BASE;
		const char *q = p+1;

		while(v < (char*)(V_MEM_BASE + V_MEM_SIZE))/*size of vedio memory */
		{
			*v++ = *q++;
			*v++ = RED_CHAR;
			if(!*q)
			{
				while(((int)v - V_MEM_BASE) % (SCREEN_WIDTH *16))
				{
					v++;
					*v++ = GRAY_CHAR;

				}
				q = p+1;
			}
		}
		__asm__ __volatile__("hlt");
	}
	while((ch = *p++) != 0)
	{
		if(ch == MAG_CH_PANIC || ch == MAG_CH_ASSERT)
			continue;/*skip the magic char */
			out_char(tty_table[p_proc->nr_tty].p_console,ch);
	}

	return 0;
}
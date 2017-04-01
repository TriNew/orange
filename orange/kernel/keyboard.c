
#include "head.h"
#include "keymap.h"
PRIVATE int 	code_with_E0 = 0;
PRIVATE int 	shift_l=0;/*此处等于0无用，在键盘初始化的地方等于0 */
PRIVATE int 	shift_r=0;
PRIVATE int 	alt_l=0;
PRIVATE int 	alt_r=0;
PRIVATE int 	ctrl_l=0;
PRIVATE int 	ctrl_r=0;
PRIVATE int 	caps_lock;
PRIVATE int 	num_lock;
PRIVATE int 	scroll_lock;
PRIVATE int 	column;


/*struct to save scan_code
typedef struct s_kb{
	char* p_head;
	char* p_tail;
	int  count;
	char buf[KB_IN_BYTES];
}KB_INPUT;
*/
PRIVATE KB_INPUT kb_in;
PRIVATE u8 get_byte_from_kbuf();
PUBLIC void keyboard_handler(int irq)
{
	/*init keyboard*/

	u8 scan_code  = in_byte(KB_DATA);
	
	if(kb_in.count < KB_IN_BYTES)
	{
		*(kb_in.p_head) = scan_code;
		kb_in.p_head++;
		if(kb_in.p_head == kb_in.buf + KB_IN_BYTES)
		{
			kb_in.p_head = kb_in.buf;
		}
		kb_in.count++;
	}

}

PUBLIC  void init_keyboard()
{
	kb_in.count = 0;
	kb_in.p_head = kb_in.buf;
	kb_in.p_tail = kb_in.buf;
	code_with_E0 = 0;
	shift_l=0;
	shift_r=0;
	alt_l=0;
 	alt_r=0;
 	ctrl_l=0;
 	ctrl_r=0;
 	/*cli and put handler to irq_table*/
	put_irq_handler(KEYBOARD_IRQ, keyboard_handler);
	enable_irq(KEYBOARD_IRQ);
}
PUBLIC void keyboard_read(TTY* p_tty)
{
	u8 scan_code;
	char output[2];
	int make=0;
	u32 key=0;
	u32* keyrow;
	//memset(output,0,2);


	if (kb_in.count>0)
	{
		disable_int();
		scan_code = *(kb_in.p_tail);
		kb_in.p_tail++;
		if(kb_in.p_tail == kb_in.buf + KB_IN_BYTES)
		{
			kb_in.p_tail = kb_in.buf;
		}
		kb_in.count--;
		enable_int();

		if(scan_code == 0xE1)
		{
			int i;
			u8 pausebrk_scode[]={0xE1,0x1D,0x45,
								 0xE1,0x9D,0xC5};
			int is_pausebreak = 1;
			for(i=1; i<6;i++)
			{
				if(get_byte_from_kbuf() != pausebrk_scode[i])
				{
					is_pausebreak =0;
					break;
				}
			}
			if(is_pausebreak)
				{key = PAUSEBREAK;}

		}
		else if(scan_code == 0xE0)
		{
				//code_with_E0 = 1;
			scan_code = get_byte_from_kbuf();
			if(scan_code == 0x2A)
			{
				if(get_byte_from_kbuf() == 0xE0)
				{
					if(get_byte_from_kbuf() == 0x37)
					{
						key = PRINTSCREEN;
						make=1;
					}
				}
			}
			if(scan_code == 0xB7)
			{
				if(get_byte_from_kbuf() == 0xE0)
				{
					if(get_byte_from_kbuf() == 0xAA)
					{
						key = PRINTSCREEN;
						make=0;
					}
				}
			}
			if(key == 0)/*中间键盘常值在第三列*/
				{code_with_E0 = 1;}
		}

		if((key != PAUSEBREAK)&&(key != PRINTSCREEN))
		{
			/*判断按下还是弹起,按下为1,弹起0
			在下文中，若是功能键,若未弹起，将功能键置1,否则将功能键置0*/
			make = (scan_code & FLAG_BREAK ? 0:1);
			keyrow = &keymap[(scan_code&0x7F)*MAP_CLOS];
			column = 0;

			if(shift_r || shift_l )
				{column=1;}
			/*中间键盘常值在第三列*/
			if(code_with_E0)
				{
					column = 2;
					code_with_E0 =0 ;
				}
/*只要按下，make总等于1*/
			key = keyrow[column];
			switch(key)
			{
				case SHIFT_L:
					shift_l = make;
					
					break;
				case SHIFT_R:
					shift_r = make;
					
					break;
				case CTRL_L:
					ctrl_l = make ;
					
				case CTRL_R:
					ctrl_r = make;
					
					break;
				case ALT_L:
					alt_l = make;
					
					break;
				case ALT_R:
					alt_r = make;
					
					break;
				default:
					break;
			}
			if(make)
			{/*将功能键键值也给key,用于高层处理key:32bits*/
				key |= shift_l ? FLAG_SHIFT_L : 0;
				key |= shift_r ? FLAG_SHIFT_R : 0;
				key |= ctrl_l ? FLAG_CTRL_L : 0;
				key |= ctrl_r ? FLAG_CTRL_R : 0;
				key |= alt_l ? FLAG_ALT_L : 0;
				key |= alt_r ? FLAG_ALT_R : 0;
				in_process(p_tty,key);
			}
			
		}
	}	
}
/*=============================================*/
PRIVATE u8 get_byte_from_kbuf()
{
	u8 scan_code;
	while(kb_in.count <= 0);
	disable_int();
	scan_code = *(kb_in.p_tail);
	kb_in.p_tail++;
	if(kb_in.p_tail == kb_in.buf + KB_IN_BYTES)
	{
		kb_in.p_tail = kb_in.buf;
	}
	kb_in.count--;
	enable_int();
	return scan_code;
}

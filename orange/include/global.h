/*EXTERN is defined as extern except in global.c*/
#ifdef GLOBAL_VARIABLES_HERE 
#undef EXTERN 
#define EXTERN  
#endif 

EXTERN int 		        disp_pos;
EXTERN u8 		        gdt_ptr[6];  /*0~15:limit   16~47:base */
EXTERN DESCRIPTOR     gdt[GDT_SIZE];
EXTERN u8 		        idt_ptr[6]; 	/*0~15:limit   16~47:base */
EXTERN GATE 	        idt[IDT_SIZE];

EXTERN PROCESS		proc_table[];
EXTERN TASK 		task_table[];
EXTERN char			task_stack[];
EXTERN TASK 		user_proc_table[];
EXTERN irq_handler        irq_table[];
EXTERN TSS			tss;
EXTERN PROCESS*  	p_proc_ready;
EXTERN system_call        sys_call_table[];

EXTERN int                     ticks;
EXTERN int                     k_reenter ;
EXTERN int                     nr_current_console; 
EXTERN TTY 		       tty_table[];
EXTERN CONSOLE 	       console_table[];
EXTERN TTY*                 p_tty;

EXTERN struct dev_drv_map dd_map[];

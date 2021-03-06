/* klib.asm */
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void	disp_str(char * info);
PUBLIC void	disp_color_str(char * info, int color);

/* protect.c */
PUBLIC void	init_prot();
PUBLIC u32	seg2phys(u16 seg);

/* klib.c */
PUBLIC void	delay(int time);

/* kernel.asm */
void restart();

/* main.c */
void TestA();
void TestB();
void TestC();
PUBLIC void init_clock();
PUBLIC  void init_keyboard();
/* i8259.c */
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);

/* clock.c */
PUBLIC void clock_handler(int irq);
PUBLIC void hd_handler(int irq);

/*proc.c*/
//PUBLIC int sys_get_ticks();
PUBLIC int send_recv(int function , int src_dest, MESSAGE* msg);
/*syscall.c*/
PUBLIC void sys_call();
PUBLIC int 	get_ticks();
/*clock.c*/
PUBLIC void milli_delay(int milli_sec);
/*keyboard.c*/
PUBLIC void in_process(TTY* p_tty,u32 key);
PUBLIC void keyboard_read();

PUBLIC void task_tty();
PUBLIC void task_sys();
PUBLIC void task_fs();
PUBLIC void task_hd();

//PUBLIC int sys_write(char* buf, int len, int unused,PROCESS* p_proc);
PUBLIC int sys_printx(int, int,char*, PROCESS*);
PUBLIC int sys_sendrec(int,int,	MESSAGE* m ,struct proc* p);
PUBLIC void spin(char *);
PUBLIC void write(char* buf , int len );


/*misc.c*/
PUBLIC int proc2pid(PROCESS *p);
/*printf.c*/
/* printf.c */
//PUBLIC  int     printf(const char *fmt, ...);
#define	printl	printf
int print1(const char *fmt,...);

/* printf.c */
PUBLIC  int     vsprintf(char *buf, const char *fmt, va_list args);
PUBLIC	int	sprintf(char *buf, const char *fmt, ...);
/*main.c*/
PUBLIC int get_ticks();
/*msg*/
PUBLIC void inform_int(int task_nr);
PUBLIC void dump_proc(struct proc* p);
PUBLIC void dump_msg(const char * title, MESSAGE* m);

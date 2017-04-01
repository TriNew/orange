#ifndef _ORANGE_CONST_H_
#define _ORANGE_CONST_H_

#ifndef ASSERT
#define ASSERT
#endif

#ifdef 	ASSERT
void 	assertion_failture(char *exp, char * file , char *base_file,int line);
#define assert(exp) if(exp) ; \
				else assertion_failture(#exp, __FILE__,__BASE_FILE__,__LINE__)
#else
#define assert(exp)
#endif


#define PUBLIC 
#define PRIVATE static 
#define EXTERN extern /*EXTERN is defined as extern except in global.c*/

#define phys_copy memcpy
/*函数类型*/

#define  GDT_SIZE 		128
#define  IDT_SIZE 		256

#define  INT_M_CTL 		0x20 
#define  INT_M_CTLMASK  0x21
#define  INT_S_CTL 		0xA0 
#define  INT_S_CTLMASK  0xA1 

/* RPL */
#define	RPL_KRNL	SA_RPL0
#define	RPL_TASK	SA_RPL1
#define	RPL_USER	SA_RPL3

#define	PRIVILEGE_KRNL	0
#define	PRIVILEGE_TASK	1
#define	PRIVILEGE_USER	3


/*可屏蔽中断个数16*/
#define NR_IRQ 			16
#define CLOCK_IRQ 		0
#define	KEYBOARD_IRQ	1
/* system call */
#define NR_SYS_CALL 	2
#define INT_VECTOR_SYS_CALL 0x90
/*8253 PIT:programmable Interval Timer*/
#define TIMER0 			0x40
#define TIMER_MODE 		0x43
#define RATE_GENERATOR 	0x34
#define TIMER_FREQ 		1193182L
#define HZ 				100
/*keyboard*/
#define MAP_CLOS		3

/* AT keyboard */
/* 8042 ports */
#define KB_DATA		0x60	/* I/O port for keyboard data
					Read : Read Output Buffer
					Write: Write Input Buffer(8042 Data&8048 Command) */
#define KB_CMD		0x64	/* I/O port for keyboard command
					Read : Read Status Register
					Write: Write Input Buffer(8042 Command) */
/*CRT*/
#define CRTC_ADDR_REG 	0x3D4
#define CRTC_DATA_REG 	0x3D5
#define START_ADDR_H 	0xC
#define START_ADDR_L	0xD
#define CURSOR_H 		0xE 
#define CURSOR_L 		0xF
#define V_MEM_BASE 		0xB8000
#define V_MEM_SIZE 		0x8000
/*TTY*/
#define NR_CONSOLES 	2
#define DEFAULT_CHAR_COLOR 0x0C

#define SCR_UP 			1
#define SCR_DN 			-1
#define SCREEN_SIZE 	(80*25)
#define SCREEN_WIDTH 	80

/* tasks */
/* 注意 TASK_XXX 的定义要与 global.c 中对应 */
#define INVALID_DRIVER	-20
#define INTERRUPT	-10
#define TASK_TTY	0
#define TASK_SYS	1
#define TASK_HD		3
/* #define TASK_WINCH	2 */
/* #define TASK_FS	3 */
/* #define TASK_MM	4 */
#define ANY		(NR_TASKS + NR_PROCS + 10)
#define NO_TASK		(NR_TASKS + NR_PROCS + 20)


#define	MAX_DRIVES		2
#define	NR_PART_PER_DRIVE	4 //the most number of patitions of a primary partition
#define	NR_SUB_PER_PART		16 //the most number of the logical partitions of each extended partition
/*total nr of logical partitions in one disk*/
#define	NR_SUB_PER_DRIVE	(NR_SUB_PER_PART * NR_PART_PER_DRIVE)
/*total nr of primary partitions in one disk*/					
#define	NR_PRIM_PER_DRIVE	(NR_PART_PER_DRIVE + 1) //include hd0
// the max number of primary partitions 
#define MAX_PRIM 			(MAX_DRIVES * NR_PRIM_PER_DRIVE - 1)
#define MAX_SUBPARTITIONS 	(NR_SUB_PER_DRIVE * MAX_DRIVES)		
/*major device numbers (corresponding to global.c::dd_map)*/
#define NO_DEV 		0
#define DEV_FLOPPY 	1
#define DEV_CDROM 	2
#define DEV_HD 		3
#define DEV_CHAR_TTY 	4
#define DEV_SCSI 	5
					
#define	P_PRIMARY	0
#define	P_EXTENDED	1

#define ORANGES_PART	0x99	/* Orange'S partition */
#define NO_PART		0x00	/* unused entry */
#define EXT_PART	0x05	/* extended partition */


/*make device number (consist of)from major and minor(次要) numbers*/
/*MAKE_DEV 将主设备号和次设备号整合到一起*/					
#define MAJOR_SHIFT 	8
#define MAKE_DEV(a,b) 	((a << MAJOR_SHIFT) | b)

/*seperate major and minor number from device number*/
#define MAJOR(x) 	((x >> MAJOR_SHIFT) & 0xFF)
#define MINOR(x)	(x & 0xFF)					
/*device number of hard disk*/
#define MINOR_hd1a 	0x10
#define MINOR_hd2a 	(MINOR_hd1a + NR_SUB_PER_PART)
/*#define MINOR_BOOT MINOR_hd2a	in config.h*/				
#define ROOT_DEV 	MAKE_DEV(DEV_HD, MINOR_BOOT)					
/***************************************************************
*harware interrupt
*/
#define	CASCADE_IRQ	2	/* cascade enable for 2nd AT controller */
#define	AT_WINT_IRQ	14



/********************************************************
*					ipc
********************************************************/
#define SEND		1
#define RECEIVE		2
/*表示将一个消息发送给目的进程，并等待目的进程将消息处理后发送给原进程 */
#define BOTH		3	
/* Process */
#define SENDING   0x02	/* set when proc trying to send */
#define RECEIVING 0x04	/* set when proc trying to recv */
/* magic chars used by `printx' */
#define MAG_CH_PANIC	'\002'
#define MAG_CH_ASSERT	'\003'
/*font color*/
#define GRAY_CHAR 	0x0F
#define RED_CHAR 	0x0C


/**
 * @enum msgtype
 * @brief MESSAGE types
 */
enum msgtype {
	/* 
	 * when hard interrupt occurs, a msg (with type==HARD_INT) will
	 * be sent to some tasks
	 */
	HARD_INT = 1,

	/* SYS task */
	GET_TICKS,

	/* message type for drivers */
	DEV_OPEN = 1001,
	DEV_CLOSE,
	DEV_READ,
	DEV_WRITE,
	DEV_IOCTL
};

#define	RETVAL		u.m3.m3i1

/********************************************************
*					string
********************************************************/
#define	STR_DEFAULT_LEN	1024
/********************************************************
*hd
*/
#define SECTOR                      512
#define SECTOR_SIZE		512
#define SECTOR_BITS		(SECTOR_SIZE * 8)
#define SECTOR_SIZE_SHIFT	9

#endif

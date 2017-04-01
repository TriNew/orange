#include "head.h"

PUBLIC void cstart()
{
	/*将loader中的GDT复制到新的GDT中*/
	memcpy(&gdt, 					/*new GDT*/
			(void*)(*((u32*)(&gdt_ptr[2]))),   /*base of old GDT*/
			*((u16*)(&gdt_ptr[0]))+1);   		/*limit of old GDT*/
	u16* p_gdt_limit = (u16*)(&gdt_ptr[0]);
	u32* p_gdt_base  = (u32*)(&gdt_ptr[2]);
	*p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1 ;
	*p_gdt_base  = (u32)&gdt; /*gdt表的起始地址,将新gdt的基地之写入gdt_ptr*/
	
	/*idt_ptr[6],*/
	u16* p_idt_limit = (u16*)(&idt_ptr[0]);
	u32* p_idt_base  = (u32*)(&idt_ptr[2]);
	*p_idt_limit = IDT_SIZE * sizeof(GATE) - 1 ;
	*p_idt_base =(u32)&idt ;/*idt表的起始地址,将idt的基地之写入idt_ptr*/
	disp_str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
			"-----------cstarting-------------------\n");
	/*在protect.c中,初始化8259A和填写idt中断向量表*/
	init_port();
	disp_str("-----------cstarted--------------------");
}
#include "head.h"

PUBLIC void init_8259A()
{
	int i ;
	for (i = 0; i < NR_IRQ; ++i)
	{
		irq_table[i] = spurious_irq;
	}

	/*Master ICW1*/
	out_byte(INT_M_CTL, 0x11);
	/*slave ICW1*/
	out_byte(INT_S_CTL, 0x11);
	
	/*ICW2,中断入口地址0x20*/
	out_byte(INT_M_CTLMASK, INT_VECTOR_IRQ0);
	out_byte(INT_S_CTLMASK, INT_VECTOR_IRQ8);
	/*ICW3*/
	out_byte(INT_M_CTLMASK, 0x4);
	out_byte(INT_S_CTLMASK, 0x2);
	/*ICW4*/
	out_byte(INT_M_CTLMASK, 0x1);
	out_byte(INT_S_CTLMASK, 0x1);
	/*OCW1*/
	out_byte(INT_M_CTLMASK, 0xFF);
	out_byte(INT_S_CTLMASK, 0xFF);

}
/*未使用的irq*/
PUBLIC void spurious_irq(int irq)
{
	disp_str("spurious_irq: ");
	disp_int(irq);
	disp_str("\n");
}
PUBLIC void put_irq_handler(int irq , irq_handler handler)
{
	disable_irq(irq);
	irq_table[irq] = handler;
}
#include "head.h"
/*****************************************************************************
 *                                spin
 *****************************************************************************/
PUBLIC void spin(char * func_name)
{
	print1("\nspinning in %s ...\n", func_name);
	while (1) ;
}

PUBLIC void assertion_failture( char * exp ,
								char *file,
								char *base_file,
								int   line)
{
	
	print1("%c  assert(%s) failed: file: %s, base_file: %s, line:%d",
			MAG_CH_ASSERT,
			exp,file,base_file,line);

	/*
		if assertion fails in a task, the system will halt before print1 return 
		if it happens in a user proc ,print1 will return like a common routine and 
		arrive here 
		@see sys_printx

		we use a forever loop to prevent the proc from going on 
	*/
	spin("assertion_failture");
	/*should never arrive here*/
	__asm__ __volatile__("ud2");
}
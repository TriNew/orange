#include "head.h"


/*<ring1> The main loop of TASK SYS
* received a msg and send it to source*/

PUBLIC void task_sys()
{
	MESSAGE msg;

	memset(&msg , 0 ,sizeof(MESSAGE));
	while(1)
	{
		send_recv(RECEIVE, ANY, &msg);
		int src = msg.source;
		
		switch(msg.type)
		{
			case GET_TICKS:
				msg.RETVAL = ticks;
				send_recv(SEND, src, &msg);
				break;
			default:
				panic("unknown msg type in task_sys");
				break;
		}
	}
}
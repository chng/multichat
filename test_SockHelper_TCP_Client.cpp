#include "SockHelper.h"

int main()
{
	TCPHelper_SendTo h("127.0.0.1", 9002);
	if(!h.connectTo())
		exit(0);
	int count = 9;
	while(count--)
	{
		h.writen("hello world\n", 13);
	}
	return 0;
}

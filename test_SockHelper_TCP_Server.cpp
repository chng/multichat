#include "SockHelper.h"
#include <sys/socket.h>

int main()
{
	char buf[1024];
	TCPHelper_Listen h(9002);
	if( !h.listenFrom() )
		exit(0);
	int fd;
	
	while(1)
	{
		TCPHelper_SendTo tmp = h.acceptFrom();
		if(fork() == 0)
		{
			if(tmp.getSocketFD() < 0)
				exit(0);
			tmp.readn(buf, sizeof(buf));
			cout <<buf<<endl;
		}
	}		
	return 0;
}

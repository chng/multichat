/*
chat broadcast: server

main: 
	create epoll
	initialize udp reader based on epoll:
	initialize tcp listener based on epoll

thread 1:
	
	while(1) 
	{
		readfrom udp
		pthread_create()
		{
			select from db
			if(has new msg)
			{
				epoll_ctl(ADD); // register a write 
			}
		}
	}
	

*/

#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <assert.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

using namespace std;

void echo_poll(int fd, sockaddr* pcliaddr, socklen_t len_client)
{
	int n;
	socklen_t lenaddr;
	char buf[1024];
	while(1)
	{
		lenaddr = len_client;
		n = recvfrom(fd, buf, sizeof(buf), 0, pcliaddr, &lenaddr);
		if(n<=0)
			continue;
		char msg[10], userid[256];
		unsigned short port;
		sscanf(buf, "%s:%s:%d", msgtype, userid, port);
		if(strcmp(msgtype, "poll")==0)
		{
			//select * from msg where to=useris and stat='unread';
			if(has new msg)
			{
				pcliaddr->sin_port = port;
				send by TCP: ip, port
			}
		}
		
	}
}

int main(int argc, char ** argv)
{
	int fd;
	struct sockaddr_in serv_addr, cli_addr;
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if( bind(fd, (sockaddr*)&serv_addr, sizeof(serv_addr)) )
	{
		cerr <<"bind error. fd = "<<fd<<"\n";
		exit(1);
	}
	echo_poll(fd, (sockaddr*)&cli_addr, sizeof(cli_addr));
}


void initialize()
{
	

}



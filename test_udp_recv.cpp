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

void echo(int fd, sockaddr* pcliaddr, socklen_t len_client)
{
	int n;
	socklen_t lenaddr;
	char buf[1024];
	while(1)
	{
		lenaddr = len_client;
		n = recvfrom(fd, buf, sizeof(buf), 0, pcliaddr, &lenaddr);

		cout <<buf<<endl;
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
	echo(fd, (sockaddr*)&cli_addr, sizeof(cli_addr));
}


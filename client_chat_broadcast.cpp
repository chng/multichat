/*
chat_broadcast: client

main:
	initialize the socket;
	initialize the UDP socket for polling : fd_udp_poll: port_poll
	initialize the TCP socket for listening : fd_tcp_listen: port_listen
	initialize the TCP socket for sending msg: fd_tcp_send: port_send
	initialize timer;
	
	run thread 1;
	run thread 2;
	run thread 3;

thread 1:
	while(1)
	{
		polling by fd_udp_poll: port_poll;
		sleeping;
	}
	polling: 
		sending polling msg to <serverip : serverport>
			polling msg: userid:port_listen

thread 2:
	listening by fd_tcp_listen: port_listen;
	if(accept())
	{
		read(msg);
		printf(msg);
	}

thread 3:
	cin >> msg;
	connect(fd_tcp_send, server_addr);
	if( 0 < write(fd_tcp_send, msg) )
		cout <<msg<<endl;
	else
		cerr<<"failed to send: " <<msg<<endl;
	shutdown(fd, SHUT_WR);
	shutdown(fd, SHUT_RD);
`	
*/
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

#define BUF_SIZE  2048

int fd_udp_poll = -1;;
int fd_tcp_listen = -1;
int fd_tcp_send = -1; 

unsigned short client_port_listen = 2001;

const char *serv_ip   = "127.0.0.1";
unsigned short serv_port_poll = 9001;
unsigned short serv_port_recv = 9002;

struct sockaddr_in serv_addr_poll;
struct sockaddr_in serv_addr_recv;
struct sockaddr_in client_addr_listen;

int sleep_poll = 2;

const char *userid = "chng";

/* function declare */
void initialize(const char * __userid, const char *__serv_ip, int __serv_port_poll, int __serv_port_recv, int __client_port_listen);
void * run_thread_0(void *);
void * run_thread_1(void *);
void * run_thread_2(void *);

void initialize(const char * __userid, const char *__serv_ip, int __serv_port_poll, int __serv_port_recv, int __client_port_listen)
{
	userid = __userid;
	serv_ip = __serv_ip;

	serv_port_poll     =  htons( __serv_port_poll );
	serv_port_recv     =  htons( __serv_port_recv );
	client_port_listen =  htons( __client_port_listen );

	
	bzero(&serv_addr_poll, sizeof(serv_addr_poll));
	serv_addr_poll.sin_family = AF_INET;
	inet_pton(AF_INET, serv_ip, &serv_addr_poll.sin_addr);
	serv_addr_poll.sin_port = serv_port_poll;

	bzero(&serv_addr_recv, sizeof(serv_addr_recv));
	serv_addr_recv.sin_family = AF_INET;
	inet_pton(AF_INET, serv_ip, &serv_addr_recv.sin_addr);
	serv_addr_recv.sin_port = serv_port_recv;

	bzero(&client_addr_listen, sizeof(client_addr_listen));
	client_addr_listen.sin_family = AF_INET;
	client_addr_listen.sin_addr.s_addr = htonl(INADDR_ANY);
	client_addr_listen.sin_port = client_port_listen;
}


int main(int argc, char ** argv)
{
	initialize(userid, serv_ip, serv_port_poll, serv_port_recv, client_port_listen);
	
	// run threads
	pthread_t tid_0, tid_1, tid_2;	// thread id
	pthread_attr_t attr;		// thread attributes
	// get the default thread attributes
	pthread_attr_init(&attr);
	
	// create thread
	pthread_create(&tid_0, &attr, run_thread_0, NULL);
	pthread_create(&tid_1, &attr, run_thread_1, NULL);
	pthread_create(&tid_2, &attr, run_thread_2, NULL);

	pthread_join(tid_0, NULL);
	pthread_join(tid_1, NULL);
	pthread_join(tid_2, NULL);

	cout <<"user "<<userid<<" exit"<<endl;
}

void * run_thread_0(void *param)
{
	cout <<"thread poll running"<<"\n";
	char buf[1024];
	sprintf(buf, "poll:%s:%d", userid, client_port_listen); 
	//cout <<buf<<endl;
	while(0 > fd_udp_poll)
	{
		fd_udp_poll = socket(AF_INET, SOCK_DGRAM, 0);
	}
	while(1)
	{
		sendto(fd_udp_poll, buf, strlen(buf), 0, (const sockaddr*)&serv_addr_poll, sizeof(serv_addr_poll));
		sleep(sleep_poll);
	}
}

void * run_thread_1(void *param)
{
	cout <<"thread listen running"<<endl;
	fd_tcp_listen = socket(PF_INET, SOCK_STREAM, 0);
	if( bind(fd_tcp_listen, (const sockaddr*)&client_addr_listen, sizeof(client_addr_listen)))
	{
		cerr <<"fd_tcp_listen bind error. fd = "<<fd_tcp_listen<<"\n";
		pthread_exit(0);
	}
	if (listen(fd_tcp_listen, 65535) )
	{
		cerr <<"fd_tcp_listen listen error fd = "<<fd_tcp_listen<<"\n";
		pthread_exit(0);
	}
	struct sockaddr_in serv_addr_send;
	int n;
	socklen_t lenaddr = sizeof(serv_addr_send);
	char buf[4096];
	while(1)
	{
		int connfd = accept(fd_tcp_listen, (sockaddr*)&serv_addr_send, &lenaddr);
		close(fd_tcp_listen);
		do
		{
			while( (n = read(connfd, buf, sizeof(buf))) > 0 )
				cout <<"<--- "<<buf<<endl;
		} while(n<0 && errno == EINTR);
		close(connfd);
	}
	
}

void * run_thread_2(void *param)
{
	cout <<"thread send running"<<"\n";
	char buf[4096];
	while(1)
	{
		buf[0]='m'; buf[1]='s'; buf[2]='g'; buf[3]=':';
		cin >> (buf+4);
		cout <<buf<<endl;
		fd_tcp_send = socket(PF_INET, SOCK_STREAM, 0);
		if(0 == connect(fd_tcp_send, (const sockaddr*)&serv_addr_recv, sizeof(serv_addr_recv)))
		{
			if(0 < send(fd_tcp_send, buf, strlen(buf), 0))
				cout <<"---> "<<buf<<endl;
			else
				cerr <<"-x-> "<<buf<<endl;
			//recv(sfd, buf, BUF_SIZE-1, 0);
			//printf("%s\n", buf);
			close(fd_tcp_send);
		}
		else
		{
			cerr << "lost connection"<<endl;
		}
	}
}


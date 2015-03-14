/*
chat broadcast: server

main: 
	epoll_create
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

/*
poll:
        poll:userid:password:listen_port(network order)

newmsg:
        newmsg:timestamp:userfrom:text

sendmsg:
        wrmsg:userid:password:text

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
#include "db.h"

using namespace std;

static UserAction ua("172.12.72.74", "root", "123", "chat_broadcast", 3306);
static MsgAction  ma("172.12.72.74", "root", "123", "chat_broadcast", 3306);

struct sockaddr_in serv_addr_poll;
struct sockaddr_in serv_addr_listen;
struct sockaddr_in client_addr_send; // recv new msg from this addr 
struct sockaddr_in client_addr_recv; // send to this addr when there is new msg. address is within client_addr_poll, and port i  within the poll packet.

static int fd_udp_poll = -1;  // UDP port, listen for polling
static int fd_tcp_send = -1;  // TCP port, sending msg
static int fd_tcp_listen = -1;// TCP port, listen for incoming msg


static int serv_port_poll = 9001;
static int serv_port_listen = 9002;


void initialize(const int __serv_port_poll, const int __serv_port_listen);
void* run_poll_handler(void *param);
int writen(int fd, char * buf, size_t len);
int readn(int fd, char * buf, size_t len);


int main(int argc, char ** argv)
{
	initialize(serv_port_poll, serv_port_listen);
	pthread_t tid0;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&tid0, &attr, run_poll_handler, NULL);
	pthread_join(tid0, NULL);
}


void initialize(const int __serv_port_poll, const int __serv_port_listen)
{
	//UserAction ua("172.12.72.74", "root", "123", "chat_broadcast", 3306);
	//MsgAction  ma("172.12.72.74", "root", "123", "chat_broadcast", 3306);

	serv_port_poll   = htons(__serv_port_poll);
	serv_port_listen = htons(__serv_port_listen);
	
	bzero(&serv_addr_poll, sizeof(sockaddr_in));
	serv_addr_poll.sin_family = AF_INET;
	serv_addr_poll.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr_poll.sin_port = serv_port_poll;
	fd_udp_poll = socket(AF_INET, SOCK_DGRAM, 0);
	if( bind(fd_udp_poll, (sockaddr*)&serv_addr_poll, sizeof(sockaddr_in)) )
	{
		cerr <<"fd_udp_poll bind error. fd = "<<fd_udp_poll<<"\n";
		pthread_exit(0);
	}
	bzero(&serv_addr_listen, sizeof(sockaddr_in));
	serv_addr_listen.sin_family = AF_INET;
	serv_addr_listen.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr_poll.sin_port = serv_port_listen;
	fd_tcp_listen = socket(AF_INET, SOCK_STREAM, 0);
}


void* run_poll_handler(void *param)
{
	int n;
	socklen_t lenaddr = sizeof(sockaddr_in);
	struct sockaddr_in client_addr_poll;
	char buf[1024];
	while(1)
	{
		lenaddr = sizeof(sockaddr_in);
		n = recvfrom(fd_udp_poll, buf, sizeof(buf), 0, (sockaddr*)&client_addr_poll, &lenaddr);
		if(n<=0)
		{
			cerr <<"read error"<<endl;
			continue;
		}
		char msgtype[10], userid[256], password[256];
		int client_port_listen;
		sscanf(buf, "%s:%s:%s:%d", msgtype, userid, password, &client_port_listen);
		cout <<buf<<endl;
		if(strcmp(msgtype, "poll")==0)
		{
			if( !ua.login(userid, password) ) 
			{
				cerr <<"received invalid polling"<<endl;
				continue;
			}
			msg *latest_msg =  ma.getLatestMsg("*");
			if(latest_msg[0].timestamp)
			{
				client_addr_recv.sin_family = AF_INET;
				client_addr_recv.sin_addr = client_addr_poll.sin_addr;
				client_addr_recv.sin_port = client_port_listen;
				fd_tcp_send = socket(AF_INET, SOCK_STREAM, 0);
				if( 0 == connect(fd_tcp_send, (const sockaddr*)&client_addr_recv, sizeof(sockaddr_in)) )
				{
					char buf[4096];
					for(int i=0; latest_msg[i].timestamp; i++)
					{
						sprintf(buf, "newmsg:%d:%s:%s", latest_msg[i].timestamp, latest_msg[i].userfrom, latest_msg[i].text);
						cout <<buf<<endl;
						writen(fd_tcp_send, buf, strlen(buf));
					}
				}
				close(fd_tcp_send);
			}
		}
		
	}
}


int writen(int fd, char * buf, size_t len)
{
	char * cur = buf;
	int n = -1;
	while(len>0)
	{
		n = write(fd, cur, len);
		if (n<=0)
		{
			if(errno == EINTR) continue;
			else return -1;
		}
		len -= n;
		cur += n;
	}
	return 0;
}

int readn(int fd, char* buf, size_t len)
{
	char *cur = buf;
	int n = -1;
	while (len>0)
	{
		n = read(fd, cur, len);
		if (n == -1)
		{
			if (errno == EINTR)
				continue;
			else break;
		}
		else if (n == 0)
			break;
		cur += n; len -= n;
	}
	return (int)(cur-buf);
}

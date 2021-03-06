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

/*
poll:
	poll:userid:password:listen_port(network order)

newmsg:
	newmsg:timestamp:userfrom:text

sendmsg:
	wrmsg:userid:password:userto:text

*/
#include "AppPacketFormat.h"
#include "SockHelper.h"

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <assert.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>

using namespace std;

static int fd_udp_poll = -1;;
static int fd_tcp_listen = -1;
static int fd_tcp_send = -1; 

unsigned short client_port_listen = 10001;

char *serv_ip   = nullptr;
unsigned short serv_port_poll = 9001;
unsigned short serv_port_listen = 9002;

//struct sockaddr_in serv_addr_poll;     // UDP listen port
static UDPHelper *cli_poll = nullptr;
//struct sockaddr_in serv_addr_listen;     // TCP listen port
//static TCPHelper_SendTo * serv_listen = nullptr;
//struct sockaddr_in client_addr_listen; // TCP listen port
static TCPHelper_Listen * cli_listen = nullptr;

int sleep_poll = 1;

char *userid = nullptr;
char * password = nullptr;

//* function declare */
int getconfig(const char *path_conf);
void initialize(const char * __userid, const char *__serv_ip, int __serv_port_poll, int __serv_port_listen, int __client_port_listen);
void * run_thread_poll(void *);
void * run_thread_listen(void *);
void * run_thread_wrmsg(void *);

int readn(int fd, char* buf, size_t len);
int writen(int fd, char * buf, size_t len);
/* end of function declare */


int main(int argc, char ** argv)
{
	if(argc < 2)
	{
		cerr <<"Usage: "<<argv[0]<<" <configuration file>"<<endl;
		exit(0);
	}
	if( 0 > getconfig(argv[1]) )
	{
		cerr <<"Configuration error"<<endl;
		exit(1);
	}
	initialize(userid, serv_ip, serv_port_poll, serv_port_listen, client_port_listen);
	
	// run threads
	pthread_t tid_0, tid_1, tid_2;	// thread id
	pthread_attr_t attr;		// thread attributes
	// get the default thread attributes
	pthread_attr_init(&attr);
	
	// create thread
	pthread_create(&tid_0, &attr, run_thread_poll, nullptr);
	pthread_create(&tid_1, &attr, run_thread_listen, nullptr);
	pthread_create(&tid_2, &attr, run_thread_wrmsg, nullptr);

	pthread_join(tid_0, nullptr);
	pthread_join(tid_1, nullptr);
	pthread_join(tid_2, nullptr);

	cout <<"user "<<userid<<" exit"<<endl;
}

void initialize(const char * __userid, const char *__serv_ip, int __serv_port_poll, int __serv_port_listen, int __client_port_listen)
{
	//userid = __userid;
	//serv_ip = __serv_ip;

	cli_poll = new UDPHelper(serv_ip, serv_port_poll);
	if(cli_poll->getSocketFD() < 0)
	{
		cerr <<"polling socket not created."<<endl;
		exit(1);
	}
	
	/*
	serv_listen = new TCPHelper_SendTo(serv_ip, serv_port_listen);	
	if(serv_listen->getSocketFD() < 0)
	{
		cerr <<"wrmsg socket not created."<<endl;
		exit(1);
	}
	*/

	cli_listen = new TCPHelper_Listen(client_port_listen);
	if(cli_listen->getSocketFD() < 0)
	{
		cerr <<"newmsg socket not created."<<endl;
		exit(1);
	}
}

void * run_thread_poll(void *param)
{
	cout <<"thread poll running"<<"\n";
	char buf[LEN_MSG_POLL];
	sprintf(buf, MSGTYPE_POLL ":%s:%s:%d", userid, password, client_port_listen); 
	while(1)
	{
		cli_poll->writen(buf, strlen(buf));
		sleep(sleep_poll);
	}
}

void * run_thread_listen(void *param)
{
	cout <<"thread listen running"<<endl;

	if ( !cli_listen->listenFrom(65535) )
	{
		cerr <<"listen for newmsg error"<<endl;
		pthread_exit(0);
	}
	int n;
	char buf[LEN_MSG_NEWMSG];
	while(1)
	{
		TCPHelper_SendTo serv_newmsg = cli_listen->acceptFrom();
		if(serv_newmsg.getSocketFD() < 0)
		{
			cerr <<"connection lost"<<endl;
			continue;
		}
		if (0 < serv_newmsg.readn(buf, sizeof(buf)) )
		{
			char msgtype[10], userfrom_name[256], userfrom[256], text[4096];
			long timestamp = 0;
			sscanf(buf, "%[^:]:%ld:%[^:]:%[^:]:%[^:]", msgtype, &timestamp, userfrom_name, userfrom, text);
			if(strcmp(MSGTYPE_NEWMSG, msgtype)==0)
			{
				if(strcmp(userid, userfrom) == 0)	
				{	
					cout <<"\nI: "<<ctime(&timestamp)<<"---> "<<text<<endl;
				}
				else
				{
					cout <<"\n"<<userfrom_name<<" ("<<userfrom<<"): "<<ctime(&timestamp)<<"<--- "<<text<<endl;
				}
			}
		}
	}
	
}

void * run_thread_wrmsg(void *param)
{
	cout <<"thread send running"<<"\n";
	char buf[LEN_MSG_WRMSG];
	while(1)
	{
		strcpy(buf, MSGTYPE_WRMSG ":");
		strcat(buf, userid);
		strcat(buf, ":");
		strcat(buf, password);
		strcat(buf, ":%:"); // to any
		int lenhead = strlen(buf);
		cin.getline(buf+lenhead, sizeof(buf)-lenhead);
		if( !buf[lenhead] )
			continue;
		TCPHelper_SendTo cli_send(serv_ip, serv_port_listen);
		if( cli_send.connectTo())
		{
			if(0 > cli_send.writen(buf, strlen(buf)))
				cerr <<"-x-> "<<buf<<endl;
		}
		else
		{
			cerr <<"no connection"<<endl;
		}
	}
}


int getconfig(const char *path_conf)
{
	ifstream fin;
	fin.open(path_conf);
	if(!fin.is_open()) {return -1;}
	string str;
	while( !fin.eof() )
	{
		fin >> str;
		if(fin.eof()) break;
		if(0==str.compare("serv_ip"))
		{
			fin >> str;
			serv_ip = new char [str.length()+1];
			if(!serv_ip) return -1;
			strcpy(serv_ip, str.data());
		}
		else if(0==str.compare("userid"))
		{
			fin >> str;
			userid = new char [str.length()+1];
			if(!userid) return -1;
			strcpy(userid, str.data());
		}
		else if(0==str.compare("password"))
		{
			fin >> str;
			password = new char [str.length()+1];
			if(!password) return -1;
			strcpy(password, str.data());
		}
		else if(0==str.compare("client_port_listen"))
		{
			fin >> str;
			client_port_listen = atoi(str.data());
			if(client_port_listen <= 0) return -1;
		}
		else if(0==str.compare("serv_port_poll"))
		{
			fin >> str;
			serv_port_poll = atoi(str.data());
			if(serv_port_poll <= 0) return -1;
		}
		else if(0==str.compare("serv_port_listen"))
		{
			fin >> str;
			serv_port_listen = atoi(str.data());
			if(serv_port_listen <= 0) return -1;
		}
		else return -1;
	}
	fin.clear();
	fin.close();
	if(!userid || !password || !serv_ip || serv_port_poll<=0 || serv_port_listen<=0 || client_port_listen<=0) return 1;
	cout <<"client_port_poll: "<<client_port_listen<<endl;
	cout <<"server ip: "<<serv_ip<<endl;
	cout <<"serv_port_poll: "<<serv_port_poll<<endl;;
	cout <<"serv_port_listen: "<<serv_port_listen<<endl;
	cout <<"Your userID: "<<userid<<endl;
	cout <<"Your password: "<<password;
	return 0;
}

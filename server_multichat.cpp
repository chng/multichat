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
        newmsg:timestamp:userfrom_name:userfrom:text

wrmsg:
        wrmsg:userid:password:userto:text

*/

#include <unistd.h>
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
#include <pthread.h>
#include "db.h"
#include <stdarg.h>
#include <string>
using namespace std;

#define MSGTYPE_POLL "poll"
#define MSGTYPE_WRMSG "wrmsg"
#define MSGTYPE_NEWMSG "newmsg"

#define LEN_MSG_POLL 55
#define LEN_MSG_NEWMSG 4096
#define LEN_MSG_WRMSG 4096

#define LEN_MSGTYPE 10
#define LEN_USERID 20
#define LEN_USERNAME 20
#define LEN_PASSWORD 20
#define LEN_TEXT 4000
#define LEN_MSG 4096


#define TRACE(X) (X)
//#define TRACE(X) (X, cout <<#X<<endl);

static char *dbhost = nullptr;
static char *dbname = nullptr;
static char *dbuser = nullptr;
static char *dbkey = nullptr;
static unsigned short dbport = 0;


static UserAction *ua = nullptr; 
static MsgAction  *ma = nullptr;

static int status = -1;

struct sockaddr_in serv_addr_poll;
struct sockaddr_in serv_addr_listen;
struct sockaddr_in client_addr_send; // recv new msg from this addr 
struct sockaddr_in client_addr_listen; // send to this addr when there is new msg. address is within client_addr_poll, and port i  within the poll packet.

static int fd_udp_poll = -1;  // UDP port, listen for polling
static int fd_tcp_send = -1;  // TCP port, sending msg
static int fd_tcp_listen = -1;// TCP port, listen for incoming msg


static int serv_port_poll = 9001; // default
static int serv_port_listen = 9002; //default


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
		if(0==str.compare("dbhost"))
		{
			fin >> str;
			dbhost = new char [str.length()+1];
			if(!dbhost) return -1;
			strcpy(dbhost, str.data());
		}
		else if(0==str.compare("dbuser"))
		{
			fin >> str;
			dbuser = new char [str.length()+1];
			if(!dbuser) return -1;
			strcpy(dbuser, str.data());
		}
		else if(0==str.compare("dbkey"))
		{
			fin >> str;
			dbkey = new char [str.length()+1];
			if(!dbkey) return -1;
			strcpy(dbkey, str.data());
		}
		else if(0==str.compare("dbname"))
		{
			fin >> str;
			dbname = new char [str.length()+1];
			if(!dbname) return -1;
			strcpy(dbname, str.data());
		}
		else if(0==str.compare("dbport"))
		{
			fin >> str;
			dbport = atoi(str.data());
			if(dbport < 0) return -1;
		}
		else if(0==str.compare("serv_port_poll"))
		{
			fin >> str;
			serv_port_poll = atoi(str.data());
			if(serv_port_poll < 0) return -1;
		}
		else if(0==str.compare("serv_port_listen"))
		{
			fin >> str;
			serv_port_listen = atoi(str.data());
			if(serv_port_listen < 0) return -1;
		}
		else return -1;
	}
	fin.clear();
	fin.close();
	if(!dbhost || !dbname || !dbuser || !dbkey || !dbport || serv_port_poll<=0 || serv_port_listen<=0) return -1;
	cout <<"db host: "<<dbhost<<endl;
	cout <<"db user: "<<dbuser<<endl;
	cout <<"db password: "<<dbkey<<endl;
	cout <<"db name: "<<dbname<<endl;
	cout <<"db port: "<<dbport<<endl;
	cout <<"port for polling (UDP): "<<serv_port_poll<<endl;
	cout <<"port for listening (TCP): "<<serv_port_listen<<endl;
	return 0;
}

void initialize(const char *dbhost, const char *dbuser, const char *dbpwd, const char *dbname, unsigned short dbport, const int __serv_port_poll, const int __serv_port_listen);
void* run_poll_handler(void *param);
void *run_recv_wrmsg(void * param);
int writen(int fd, char * buf, size_t len);
int readn(int fd, char * buf, size_t len);

void program_exit(int n);


int main(int argc, char ** argv)
{
	if(argc < 2)
	{
		cerr <<"Usage: "<<argv[0]<<" <configure file>"<<endl;
		program_exit(0);
	}
	if(0 > getconfig( argv[1]) )
	{
		cerr <<"Configuration error."<<endl;
		program_exit(1);
	}
	initialize(dbhost, dbuser, dbkey, dbname,  dbport, serv_port_poll, serv_port_listen);
	pthread_t tid0, tid1;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&tid0, &attr, run_poll_handler, nullptr);
	pthread_create(&tid1, &attr, run_recv_wrmsg, nullptr);
	pthread_join(tid0, nullptr);
	pthread_join(tid1, nullptr);
}


void initialize(const char *dbhost, const char *dbuser, const char *dbpwd, const char *dbname, unsigned short dbport, const int __serv_port_poll, const int __serv_port_listen)
{
	ua = new UserAction(dbhost, dbuser, dbpwd, dbname, dbport);
	if(!ua || !ua->isready())
	{
		cerr <<"no connection with the database."<<endl;
		program_exit(1);
	}
	ma = new MsgAction(dbhost, dbuser, dbpwd, dbname, dbport);
	
	if( -1 == (status = ua->getUserCount()) )
	{
		cerr <<"internal error. check the database or network connection."<<endl;
		program_exit(1);
	}

	cout <<"server started."<<endl;
	serv_port_poll   = htons(__serv_port_poll);
	serv_port_listen = htons(__serv_port_listen);
	
	bzero(&serv_addr_poll, sizeof(sockaddr_in));
	serv_addr_poll.sin_family = AF_INET;
	serv_addr_poll.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr_poll.sin_port = serv_port_poll;
	fd_udp_poll = socket(AF_INET, SOCK_DGRAM, 0);
	if( bind(fd_udp_poll, (sockaddr*)&serv_addr_poll, sizeof(sockaddr_in)) )
	{
		cerr <<"fd_udp_poll bind error. fd = "<<fd_udp_poll<<endl;
		program_exit(1);
	}

	bzero(&serv_addr_listen, sizeof(sockaddr_in));
	serv_addr_listen.sin_family = AF_INET;
	serv_addr_listen.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr_listen.sin_port = serv_port_listen;
	fd_tcp_listen = socket(AF_INET, SOCK_STREAM, 0);
	if( bind(fd_tcp_listen, (sockaddr*)&serv_addr_listen, sizeof(sockaddr_in)) )
	{
		cerr <<"fd_tcp_listen bind error. fd = "<<fd_tcp_listen<<endl;
		program_exit(1);
	}
	
}


void* run_poll_handler(void *param)
{
	cout <<"run_poll_handler is runing"<<endl;
	int n;
	socklen_t lenaddr = sizeof(sockaddr_in);
	struct sockaddr_in client_addr_poll;
	char buf[LEN_MSG_POLL];
	while(1)
	{
		lenaddr = sizeof(sockaddr_in);
		n = recvfrom(fd_udp_poll, buf, sizeof(buf), 0, (sockaddr*)&client_addr_poll, &lenaddr);
		if(n<=0)
		{
			cerr <<"read error"<<endl;
			continue;
		}
		char msgtype[LEN_MSGTYPE], userid[LEN_USERID], password[LEN_PASSWORD];
		int client_port_listen;
		sscanf(buf, "%[^:]:%[^:]:%[^:]:%d", msgtype, userid, password, &client_port_listen);
		if(strcmp(msgtype, MSGTYPE_POLL)==0)
		{
			if( !ua->login(userid, password) ) 
			{
				cerr <<"received invalid polling: "<<userid<<" "<<password<<endl;
				continue;
			}
			const msg_name *latest_msg = 0;
			latest_msg = ma->getLatestMsg(userid, "%");
			if(!latest_msg || !latest_msg[0].userfrom_name)
			{
				continue;
			}

			client_addr_listen.sin_family = AF_INET;
			client_addr_listen.sin_addr = client_addr_poll.sin_addr;
			client_addr_listen.sin_port = client_port_listen;
			fd_tcp_send = socket(AF_INET, SOCK_STREAM, 0);
			if( !connect(fd_tcp_send, (const sockaddr*)&client_addr_listen, sizeof(sockaddr_in)) )
			{
				char buf[LEN_MSG_NEWMSG];
				for(int i=0; latest_msg[i].timestamp; i++)
				{
					sprintf(buf, MSGTYPE_NEWMSG ":%d:%s:%s:%s", latest_msg[i].timestamp, latest_msg[i].userfrom_name, latest_msg[i].userfrom, latest_msg[i].text);
					writen(fd_tcp_send, buf, strlen(buf));
				}
			}
			ma->freeResult();
			close(fd_tcp_send);
			delete latest_msg;
		}
		
	}
}

void *run_recv_wrmsg(void * param)
{
	cout <<"run_recv_wrmsg is runing"<<endl;
	if( listen(fd_tcp_listen, 65535) )
	{
		cerr <<"fd_tcp_listen error"<<endl;
		pthread_exit(0);
	}
	 
	int n;
	socklen_t lenaddr = sizeof(sockaddr_in);
	char buf[LEN_MSG_WRMSG];
	while(1)
	{
		lenaddr = sizeof(sockaddr_in);
		int connfd = TRACE(accept(fd_tcp_listen, (sockaddr*)&client_addr_send, &lenaddr));
		if(connfd<0)
		{
			cerr <<"conndfd error"<<endl;
			continue;
		}
		if(readn(connfd, buf, sizeof(buf)) > 0 )
		{
			char msgtype[LEN_MSGTYPE], userid[LEN_USERID], password[LEN_PASSWORD], userto[LEN_USERID], text[LEN_TEXT];
			sscanf(buf, "%[^:]:%[^:]:%[^:]:%[^:]:%[^:]", msgtype, userid, password, userto, text);
			cout <<buf<<endl;
			if(strcmp(MSGTYPE_WRMSG, msgtype)==0 && ua->login(userid, password))
				ma->insertNewMsg(userid, userto, text, status);
		}
		close(connfd);
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
	*cur = 0;
	return (int)(cur-buf);
}

void program_exit(int n)
{
	if(ua) delete ua;
	if(ma) delete ma;
	exit(n);
}


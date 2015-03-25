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
#include "SockHelper.h"
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


#define DEGUB(X) cout<<#X<<" = "<<X<<endl;
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

static UDPHelper *serv_poll = nullptr;
static TCPHelper_Listen *serv_listen = nullptr;

//static int fd_udp_poll = -1;  // UDP port, listen for polling
static int fd_tcp_send = -1;  // TCP port, sending msg
//static int fd_tcp_listen = -1;// TCP port, listen for incoming msg


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


void initialize(const char *dbhost, const char *dbuser, const char *dbpwd, const char *dbname, unsigned short dbport, const int _serv_port_poll, const int _serv_port_listen)
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

	// UDPHelper (for recvfrom)
	serv_poll = new UDPHelper(serv_port_poll);
	if(serv_poll->getSocketFD() < 0)
	{
		cerr <<"serv_poll not created."<<endl;
		program_exit(1);
	}

	//TCPHelper Listen
	serv_listen = new TCPHelper_Listen(serv_port_listen);
	if(serv_listen->getSocketFD() < 0)
	{
		cerr <<"serv_listen not created."<<endl;
		program_exit(1);
	}
}


void * run_poll_handler(void *param)
{
	cout <<"run_poll_handler is runing ("<<serv_port_poll<<")"<<endl;
	int n;
	char buf[LEN_MSG_POLL];
	while(1)
	{
		n = serv_poll->readn(buf, sizeof(buf));
		DEBUG(buf);
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
			//TCPHelper_Send
			SA_IN cli_addr = serv_poll->getRemoteAddr();
			TCPHelper_SendTo to_cli(cli_addr.sin_addr, client_port_listen);
			if( to_cli.connectTo() )
			{
				char buf[LEN_MSG_NEWMSG];
				for(int i=0; latest_msg[i].timestamp; i++)
				{
					sprintf(buf, MSGTYPE_NEWMSG ":%d:%s:%s:%s", latest_msg[i].timestamp, latest_msg[i].userfrom_name, latest_msg[i].userfrom, latest_msg[i].text);
					to_cli.writen(buf, strlen(buf));
				}
			}
			ma->freeResult();
			delete latest_msg;
		}
		
	}
}

void *run_recv_wrmsg(void * param)
{
	cout <<"run_recv_wrmsg is runing"<<endl;
	if( !serv_listen->listenFrom(65535) )
	{
		cerr <<"serv_listen error"<<endl;
		pthread_exit(0);
	}
	 
	int n;
	char buf[LEN_MSG_WRMSG];
	while(1)
	{
		TCPHelper_SendTo to_cli = TRACE(serv_listen->acceptFrom());
		if(to_cli.getSocketFD() < 0)
		{
			cerr <<"conndfd error"<<endl;
			continue;
		}
		if( to_cli.readn(buf, sizeof(buf)) > 0 )
		{
			char msgtype[LEN_MSGTYPE], userid[LEN_USERID], password[LEN_PASSWORD], userto[LEN_USERID], text[LEN_TEXT];
			sscanf(buf, "%[^:]:%[^:]:%[^:]:%[^:]:%[^:]", msgtype, userid, password, userto, text);
			cout <<buf<<endl;
			if(strcmp(MSGTYPE_WRMSG, msgtype)==0 && ua->login(userid, password))
				ma->insertNewMsg(userid, userto, text, status);
		}
	}
}

void program_exit(int n)
{
	if(ua) delete ua, ua = nullptr;
	if(ma) delete ma, ma = nullptr;
	exit(n);
}


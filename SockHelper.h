
// SockHelper is a ADT for UDP/TCP socket for listening/sending

#include <fcntl.h>
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
#include <string>
using namespace std;


typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;
typedef unsigned short port_t;

/************************************************************/
/*                  TCP/UDP Socket ADT                      */
/************************************************************/

//abstract parent class for sockethelper_listen and sockethelp_send
class SockHelper
{
public:


protected:
	SA_IN addr;
	port_t port;
	int sockfd;

	// make it uninstantiated
	virtual void IamAbstract() = 0;

public:
	
	// Where should I placed the close() function?
	virtual ~SockHelper()
	{
		close(sockfd);
	}
	// initialize by sockfd and addr
	SockHelper(int _sockfd, const SA_IN &_addr):sockfd(_sockfd), addr(_addr){}
	
	// socktype: SOCK_DGRAM / SOCK_STREAM
	// if the addr is not specified, the constructor will create a listening socket and bind it.
	SockHelper( port_t _port, int socktype):sockfd(-1), port(_port)
	{
		bzero(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		if(socktype == SOCK_DGRAM)
			sockfd = socket( AF_INET, socktype, 0 );
		else if(socktype == SOCK_STREAM)
			sockfd = socket( AF_INET, socktype, 0 );
		if( sockfd>=0 && bind( sockfd, (const SA* )&addr, sizeof(SA_IN) ))
		{
			close(sockfd);
			sockfd  = -1;
		}
	}
	// else if the addr is specified, the constructor will create a socket for sending data.
	// the addr canbe given by const string or struct in_addr.
	SockHelper(const char *ip_str, port_t _port, int socktype):sockfd(-1), port(_port)
	{
		bzero(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		inet_pton( addr.sin_family, ip_str, &addr.sin_addr );
		if(socktype == SOCK_STREAM || socktype == SOCK_DGRAM)
			sockfd = socket( AF_INET, socktype, 0 );
	}
	SockHelper(struct in_addr &_in_addr, port_t _port, int socktype):sockfd(-1), port(_port)
	{
		bzero(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr = _in_addr;
		if(socktype == SOCK_STREAM || socktype == SOCK_DGRAM)
			sockfd = socket( AF_INET, socktype, 0 );
	}


	int getSocketFD() { return sockfd; }

	bool setSocketNonBlocking()
	{
		int flags = -1;
		if(-1 == (flags = fcntl(sockfd, F_GETFL, 0)))
		{
			sockfd = -1;
			return false;
		}
		flags |= O_NONBLOCK;
		if( fcntl(sockfd, F_SETFL, flags) == -1 )
		{
			sockfd = -1;
			return false;
		}
		return true;
	}
};

// TCP socket uses read/write or send/recv
// UDP socket uses recvfrom/sendto

class SockHelper_TCP: public SockHelper
{
	virtual void IamAbstract(){}
public:	
	
	virtual ~SockHelper_TCP(){}
	
	SockHelper_TCP(int _sockfd, const SA_IN &_addr):SockHelper(_sockfd, _addr){}

	SockHelper_TCP(port_t _port):SockHelper(_port, SOCK_STREAM){}
	SockHelper_TCP(const char *ip, port_t _port):SockHelper(ip, _port, SOCK_STREAM){}
	SockHelper_TCP(struct in_addr &_inaddr, port_t _port):SockHelper(_inaddr, _port, SOCK_STREAM){}
};

class SockHelper_UDP: public SockHelper
{
	virtual void IamAbstract(){}
public:
	
	virtual ~SockHelper_UDP(){}

	SockHelper_UDP(port_t _port):SockHelper(_port, SOCK_DGRAM){}	
	SockHelper_UDP(const char *ip, port_t _port):SockHelper(ip, _port, SOCK_DGRAM){}
	SockHelper_UDP(struct in_addr &_inaddr, port_t _port):SockHelper(_inaddr, _port, SOCK_DGRAM){}

};


/************************************************************/
/*                  Send/Recv ADT                           */
/************************************************************/

class I_RWHelper
{
public:
	virtual int readn (char * buf, size_t len) = 0;
	virtual int writen(char * buf, size_t len) = 0;
};


class RWHelper_TCP: public I_RWHelper
{
public:
};

class RWHelper_UDP: public I_RWHelper
{
public:
};


/************************************************************/
/*                  Send/Recv ADT                           */
/************************************************************/

// addr should be specified.
class TCPHelper_SendTo: public SockHelper_TCP, I_RWHelper
{
public:
	~TCPHelper_SendTo(){}

	TCPHelper_SendTo(int _sockfd, const SA_IN &_addr):SockHelper_TCP(_sockfd, _addr){}

	TCPHelper_SendTo(const char *ip, port_t _port):SockHelper_TCP(ip, _port){}
	TCPHelper_SendTo(struct in_addr &_inaddr, port_t _port):SockHelper_TCP(_inaddr, _port){}

	bool connectTo()
	{
		if( 0 == connect(sockfd, (const SA *)&addr, sizeof(SA_IN)) )
			return true;
		return false;
	}

	int writen(char *buf, size_t len)
	{
		int fd = sockfd;
		if(fd<0) return -1;
		// if sockfd is invalid
		char *cur = buf;
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

	int readn(char *buf, size_t len)
	{
		int fd = sockfd;
		if(fd<0) return -1;
		// if sockfd is invalid
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
};

// addr should be ANY
class TCPHelper_Listen: public SockHelper_TCP 
{
public:
	~TCPHelper_Listen(){}
	TCPHelper_Listen(port_t _port):SockHelper_TCP(_port){}

	bool listenFrom()
	{
		if( listen(sockfd, port) )
		{
			sockfd = -1;
			return false;
		}
		return true;
	}

	TCPHelper_SendTo acceptFrom()
	{
		SA_IN cli_addr;
		socklen_t lenaddr = sizeof(SA_IN);
		int fd = accept(sockfd, (SA *)&cli_addr, &lenaddr);
		return TCPHelper_SendTo(fd, cli_addr);
	}
};


// UDP socket cannot listen
class UDPHelper: public SockHelper_UDP, I_RWHelper
{
public:	
	~UDPHelper(){}
	UDPHelper(port_t _port):SockHelper_UDP(_port){}	
	UDPHelper(const char *ip, port_t _port):SockHelper_UDP(ip, _port){}
	UDPHelper(struct in_addr &_inaddr, port_t _port):SockHelper_UDP(_inaddr, _port){}

	int readn (char *buf, size_t len)
	{
		int fd = sockfd;
		socklen_t lenaddr = sizeof(SA_IN);
		return recvfrom(fd, buf, len, 0, (SA*)&addr, &lenaddr); 
	}
	
	int writen(char *buf, size_t len)
	{
		int fd = sockfd;
		socklen_t lenaddr = sizeof(SA_IN);
		return sendto(fd, buf, len, 0, (SA *)&addr, lenaddr);	
	}
};


/*
int main()
{
	//TDPHelper_Listen
	//TCPHelper_SendTo
	//UDPHelper

	TCPHelper_Listen h_tcplisten(9001);
	TCPHelper_SendTo h_tcpsend("127.0.0.1", 9002);
	UDPHelper h_udplisten(9003);
	UDPHelper h_udp("127.0.0.1", 9004);
	SockHelper *h = nullptr;

	h = &h_tcplisten;
	cout <<h->getSocketFD()<<endl;
	h = &h_tcpsend;
	cout <<h->getSocketFD()<<endl;
	h = &h_udplisten;
	cout <<h->getSocketFD()<<endl;
	h = &h_udp;
	cout <<h->getSocketFD()<<endl;
	
	return 0;
}
// */


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

#define DEBUG(X) cout<<#X<<" = "<<X<<endl
//#define DEBUG(X) 

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;
typedef unsigned short port_t;

/************************************************************/
/*                  TCP/UDP Socket ADT                      */
/************************************************************/

#ifndef SOCKHELPER
#define SOCKHELPER
//abstract parent class for sockethelper_listen and sockethelp_send
class SockHelper
{

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
	// initialize by sockfd and addr (for special usage)
	SockHelper(int _sockfd, const SA_IN &_addr):sockfd(_sockfd), addr(_addr){}
	
	// socktype: SOCK_DGRAM / SOCK_STREAM
	// if the addr is not specified, the constructor will create a listening socket and bind it.
	SockHelper( port_t _port, int socktype):sockfd(-1), port(_port)
	{
		bzero(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		if(socktype == SOCK_DGRAM || socktype == SOCK_STREAM)
			sockfd = socket( AF_INET, socktype, 0 );
		if( sockfd>=0 && bind( sockfd, (const SA* )&addr, sizeof(SA_IN) ))
		{
			close(sockfd);
			sockfd  = -1;
		}
		cerr <<"socket is create : "<<sockfd<<endl;
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
#endif


#ifndef SOCKHELPER_TCP
#define SOCKHELPER_TCP
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

#endif


#ifndef SOCKHELPER_UDP
#define SOCKHELPER_UDP
class SockHelper_UDP: public SockHelper
{
	virtual void IamAbstract(){}
public:
	
	virtual ~SockHelper_UDP(){}

	SockHelper_UDP(port_t _port):SockHelper(_port, SOCK_DGRAM){}	
	SockHelper_UDP(const char *ip, port_t _port):SockHelper(ip, _port, SOCK_DGRAM){}
	SockHelper_UDP(struct in_addr &_inaddr, port_t _port):SockHelper(_inaddr, _port, SOCK_DGRAM){}

};
#endif


/************************************************************/
/*                  Send/Recv ADT                           */
/************************************************************/

// TCP socket uses read/write or send/recv
// UDP socket uses recvfrom/sendto

#ifndef I_RWHELPER
#define I_RWHELPER
class I_RWHelper
{
protected:
	int fd;
	void setFD(int FD) {fd = FD;}
public:
	I_RWHelper(){}
	I_RWHelper(int _fd):fd(_fd){}
	virtual int readn (char * buf, size_t len) = 0;
	virtual int writen(char * buf, size_t len) = 0;
};
#endif



#ifndef RWHELPER_TCP
#define RWHELPER_TCP
// ?
class RWHelper_TCP: public I_RWHelper
{
public:
	int writen(char *buf, size_t len)
	{
		//int fd = sockfd;
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
		//int fd = sockfd;
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
#endif

#ifndef RWHELPER_UDP
#define RWHELPER_UDP
// ?
class RWHelper_UDP: public I_RWHelper
{
	SA_IN addrremote;

protected:
	void setRemoteAddr(SA_IN &addr)
	{
		addrremote = addr;
	}

public:
	
	int readn (char *buf, size_t len)
	{
		//int fd = sockfd;
		socklen_t lenaddr = sizeof(SA_IN);
		return recvfrom(fd, buf, len, 0, (SA*)&addrremote, &lenaddr); 
	}
	
	int writen(char *buf, size_t len)
	{
		//int fd = sockfd;
		socklen_t lenaddr = sizeof(SA_IN);
		return sendto(fd, buf, len, 0, (SA *)&addrremote, lenaddr);	
	}

	SA_IN getRemoteAddr()
	{
		return addrremote;
	}
};
#endif

/************************************************************/
/*                      Final ADT                           */
/************************************************************/

#ifndef TCPHELPER_SENDTO
#define TCPHELPER_SENDTO 
// addr should be specified.
class TCPHelper_SendTo: public SockHelper_TCP, public RWHelper_TCP
{
public:
	~TCPHelper_SendTo(){}

	TCPHelper_SendTo(int _sockfd, const SA_IN &_addr):SockHelper_TCP(_sockfd, _addr)
	{
		setFD(_sockfd);
	}

	TCPHelper_SendTo(const char *ip, port_t _port):SockHelper_TCP(ip, _port)
	{
		setFD(sockfd);
	}

	TCPHelper_SendTo(struct in_addr &_inaddr, port_t _port):SockHelper_TCP(_inaddr, _port)
	{
		setFD(sockfd);
	}

	bool connectTo()
	{
		if( sockfd >=0 && !connect(sockfd, (const SA *)&addr, sizeof(SA_IN)) )
			return true;
		return false;
	}
};
#endif


#ifndef TCPHELPER_LISTEN
#define TCPHELPER_LISTEN 
// addr should be ANY
class TCPHelper_Listen: public SockHelper_TCP 
{
public:
	~TCPHelper_Listen(){}
	TCPHelper_Listen(port_t _port):SockHelper_TCP(_port){}

	bool listenFrom(unsigned BACKLOG)
	{
		if( listen(sockfd, BACKLOG) )
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
#endif


#ifndef UDPHELPER
#define UDPHELPER
// UDP socket cannot listen
class UDPHelper: public SockHelper_UDP, public RWHelper_UDP

{
public:	
	~UDPHelper(){}
	UDPHelper(port_t _port):SockHelper_UDP(_port)
	{
		setFD(sockfd);
		//setRemoteAddr(addr);
		DEBUG(fd);
	}	
	UDPHelper(const char *ip, port_t _port):SockHelper_UDP(ip, _port)
	{
		setFD(sockfd);
		setRemoteAddr(addr);
		DEBUG(fd);
	}
	UDPHelper(struct in_addr &_inaddr, port_t _port):SockHelper_UDP(_inaddr, _port)
	{
		setFD(sockfd);
		setRemoteAddr(addr);
		DEBUG(fd);
	}
};
#endif


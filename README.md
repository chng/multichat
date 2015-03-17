# multichat

Description:
multichat consists of a server, a remote mysql db and client. 
Each client will receive the latest msg sent from any other clients .

Architecture:
|+++++++++++++++++++++++++++++++++++++++++|
|                                         |
|         |--------|         |--------|   |
|         | server | <====>  |  MySQL |   |
|         |________|         |________|   |
| hello /|     |hello \   hello           |
|     /       \|/       \|                |
| client0    client1    client2           |
|                                         |
|+++++++++++++++++++++++++++++++++++++++++|


====================================================================

Sinple Protocol:

poll:
        poll:userid:password:listen_port(network order)

newmsg:
        newmsg:timestamp:userfrom_name:userfrom:text

sendmsg:
        wrmsg:userid:password:userto:text


====================================================================
MySQL DB:

user: |userid : username|
msg : |timestamp : from : to : text|

In the server program, we use <b>3-tier architecture</b>. 
Model:
          struct user
          {
                    string userid;
                    string username;
          };
          struct msg
          {
                    int timestamp;
                    string from;
                    string to;
                    string text;
          }

DAL:
	  //this is a <b>singleton</b> class, thus it is shared by BLL classes.
          class CMYSQL_Singleton
          {
                    
          };

BSS:
          class UserOperation
          {
                    // for validation
          };
          class MsgOperation
          {
                    // for pushing msg
          };

====================================================================
Client: client consists of <b>three pthreads</b>: 
thread 0: polling (UDP)
thread 1: listening for new msg (TCP)
thread 2: waiting for input, and sending the input to the server.

Server v1: serverv v1 is syncronized, and consists of two pthread:
thread 0: read polling packets (UDP recvfrom, BLOCKING)
          pthread_create
          {
            selecthing for the latest msg for the user
            sending it by TCP socket.
          }
thread 1: listening for the sent msg from client.
          {
            pthread_create
            {
              insert to the DB.
            }
          }
thread 2:waiting for input, and send the input as wrmsg.
	 
          
====================================================================

Server v2: v2 leverages epoll, based on event. using UNBLOCK UDP&TCP socket. 
main loop: 
	  epoll_create();
	  while(1)
	  {
	  	epoll_wait()
	  	forech(event)
	  	{
	  		if(read event)
	  		{
	  			if(poll) 
	  			{
	  				select;
	  				epoll_ctl(add, write);
	  			}
	  			else if(wrmsg)
	  			{
	  				insert into mysql;
	  			}
	  		}
	  		else if(write)
	  		{
	  			if(newmsg) send;
	  		}
	  	}
	  }

====================================================================

FUTURE IMPROVE:
0  use epoll and O_NONBLOCK socket. 
1  consider proxy between several servers and the clients. use consistant hash (ip as key) for load balance. 
2  consider proxy between the servers and multiple mysql servers. use consistant hash (ip as key) for load balance. 
3  multiple process. master-worker model. need to consider database connection lock and thundering herd.
4  msg encryption
5  setup log module

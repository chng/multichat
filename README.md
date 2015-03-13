# chat_broadcast

Description:
chat_broadcast consists of a server, a remote mysql db and client. 
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

MySQL DB:
user: |userid : username|
msg : |timestamp : from : to : text|

Client:
client consists of three pthreads: 
thread 0: polling (UDP)
thread 1: listening for new msg (TCP)
thread 2: waiting for input, and sending the input to the server.

Server v1:
serverv v1 is syncronized, and consists of two pthread:
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
          

Server v2:
v2 leverages epoll, based on event. 
thread 0: read polling packets. (BLOCKING)
          pthread_create
          {
            select;
            epoll_ctl(efd, ADD, WRITE);
          }
          pthread_create
          {
            while(1)
              epoll_wait(efd, WRITE);
              while(...)
                send_handler();
          }

Server v3"
using UNBLOCK UDP socket, and recv polling packets by epoll.

FUTURE IMPROVE:
0  eliminate pthread both in client and server. 
1  proxy between several servers and the clients.
2  proxy between the servers and the mysql server cluster.

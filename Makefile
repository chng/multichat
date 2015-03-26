CC = g++ 
CFLAGS = -std=c++11 -g -O2
LIB_MYSQL = `mysql_config --cflags --include --libs` -L/usr/lib -L/usr/lib/mysql
LIB_PTHREAD = -lpthread


test_cmysql:
	$(CC) test_cmysql.cpp $(LIB_MYSQL) $(CFLAGS) -o test_cmysql.bin

test_sockhelper:
	$(CC) SockHelper.h test_SockHelper_TCP_Server.cpp $(CFLAGS) -o test_SockHelper_TCP_Server.bin
	$(CC) SockHelper.h test_SockHelper_TCP_Client.cpp $(CFLAGS) -o test_SockHelper_TCP_Client.bin
	$(CC) SockHelper.h test_SockHelper_UDP_Recv.cpp $(CFLAGS) -o test_SockHelper_UDP_Recv.bin
	$(CC) SockHelper.h test_SockHelper_UDP_Send.cpp $(CFLAGS) -o test_SockHelper_UDP_Send.bin

client:
	$(CC) AppPacketFormat.h SockHelper.h client_multichat.cpp $(LIB_PTHREAD) $(CFLAGS) -o client_multichat.bin

server:
	$(CC) AppPacketFormat.h SockHelper.h singleton.h db.h server_multichat.cpp $(LIB_MYSQL) $(LIB_PTHREAD) $(CFLAGS) -o server_multichat.bin

clean:
	rm -rf *.a *.o *.so *.bin *.out

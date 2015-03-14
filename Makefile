CC = g++ 
CFLAGS = -std=c++11 -g
LIB_MYSQL = `mysql_config --cflags --include --libs` -L/usr/lib -L/usr/lib/mysql
LIB_PTHREAD = -lpthread

CPP_SRC = $(*.cpp)
CPP_OBJ = $(%cpp, %o, $(CPP_SRC))


cmysql:
	$(CC) test_c_mysql.cpp $(LIB_MYSQL) $(CFLAGS) -o test_c_mysql.bin

client:
	$(CC) client_chat_broadcast.cpp $(LIB_PTHREAD) $(CFLAGS) -o client_chat_broadcast.bin

server:
	$(CC) db.h server_chat_broadcast.cpp $(LIB_MYSQL) $(LIB_PTHREAD) $(CFLAGS) -o server_chat_broadcast.bin

clean:
	rm -f *.a *.o *.so

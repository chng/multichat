CC = g++ 
CFLAGS = -std=c++11 -g
LIB_MYSQL = `mysql_config --cflags --include --libs` -L/usr/lib -L/usr/lib/mysql
LIB_PTHREAD = -lpthread

CPP_SRC = $(*.cpp)
CPP_OBJ = $(%cpp, %o, $(CPP_SRC))


cmysql:
	$(CC) test_c_mysql.cpp $(LIB_MYSQL) $(CFLAGS) -o test_c_mysql.bin

client:
	$(CC) SockHelper.h client_multichat.cpp $(LIB_PTHREAD) $(CFLAGS) -o client_multichat.bin

server:
	$(CC) SockHelper.h singleton.h db.h server_multichat.cpp $(LIB_MYSQL) $(LIB_PTHREAD) $(CFLAGS) -o server_multichat.bin

clean:
	rm -rf *.a *.o *.so *.bin *.out

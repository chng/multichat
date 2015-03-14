cmysql:
	g++ test_c_mysql.cpp `mysql_config --cflags --include --libs` -L/usr/lib -L/usr/lib/mysql -g -o test_c_mysql.bin -std=c++11

client:
	g++ client_chat_broadcast.cpp -lpthread -g -o client_chat_broadcast.bin

server:
	g++ server_chat_broadcast.cpp `mysql_config --cflags --include --libs` -L/usr/lib -L/usr/lib/mysql -lpthread -g -o server_chat_broadcast.bin

#include <mysql.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>

using namespace std;

/* Model */
struct user
{
	char *userid;
	char *username;
	char *password;
};

struct msg
{
	bool status;
	int  timestamp;
	char *userfrom;
	char *userto;
	char *text;
};

/* DAL */
class CMYSQL
{
	MYSQL *conn;
	char *dbhost;
	char *username;
	char *key;
	char *dbname;
	unsigned int port;
	bool isconn;
	static CMYSQL* pmysql;

	CMYSQL( const char *__dbhost, 
		const char *__username,
		const char *__key,
		const char *__dbname,
		const unsigned int __port
		)
	{
		dbhost = new char[strlen(__dbhost)];
		strcpy(dbhost, __dbhost);
		username = new char[strlen(__username)];
		strcpy(username, __username);
		key = new char[strlen(__key)];
		strcpy(key, __key);
		dbname = new char[strlen(__dbname)];
		strcpy(dbname, __dbname);
		isconn = false;
		port = __port;
		conn = mysql_init(0);
	}
	~CMYSQL()
	{
		if(dbhost) delete dbhost;
		dbhost = NULL;
		if(username) delete username;
		username = NULL;
		if(key) delete key;
		key = NULL;
		if(dbname) delete dbname;
		dbname = NULL;
		if(conn)  mysql_close(conn);
		conn = NULL;
	}
public:
	
	class GC
	{
	public: 
	~GC()
	{
		if(pmysql) delete pmysql;
	}
	};
	static GC gc;
	
	static CMYSQL *getInstance( 	const char *__dbhost, 
				const char *__username,
				const char *__key,
				const char *__dbname,
				const unsigned int __port
		)
	{
		if(!pmysql)
			pmysql = new CMYSQL (__dbhost, __username, __key, __dbname, __port);
		return pmysql;
	}
	CMYSQL *getInstance()
	{
		return pmysql;
	}

	MYSQL * connect()
	{
		if(!isconn)
		{
			conn = mysql_real_connect(conn, dbhost, username, key, dbname, port, NULL, 0);
			if(conn) isconn = true;
		}
		return conn;
	}
	
	void close()
	{
		if(isconn)
		{
			mysql_close(conn);
			isconn = false;
		}
	}
	
	MYSQL_RES * query(const char *str_query)
	{
		if(!isconn)
			return NULL;
		if( mysql_query(conn, str_query) )
			return NULL;
		return mysql_store_result(conn);
	}

};
CMYSQL * CMYSQL::pmysql = NULL;


/* BLL */
class UserAction
{
	CMYSQL * pmysql;
public:
	UserAction(const char *dbhost, const char *username, const char *key, const char *dbname, unsigned int port)
	{
		pmysql = CMYSQL::getInstance(dbhost, username, key, dbname, port);
		pmysql->connect();
	}
	~UserAction()
	{
		pmysql->close();
	}

	bool login(const char *userid, const char *key)
	{
		char str_query[100];
		sprintf(str_query, "select COUNT(*) from user where userid='%s' and password='%s';", userid, key);
		MYSQL_RES *res = pmysql->query(str_query);
		
		if(res)
		{
			MYSQL_ROW row = mysql_fetch_row(res);
			if(atoi(row[0])) return true;
		}
		mysql_free_result(res);
		return false;
	}
};

class MsgAction
{
	CMYSQL * pmysql;
public:
	MsgAction(const char *dbhost, const char *username, const char *key, const char *dbname, unsigned int port)
	{
		pmysql = CMYSQL::getInstance(dbhost, username, key, dbname, port);
		pmysql->connect();
	}
	~MsgAction()
	{
		pmysql->close();
	}

	msg* getLatestMsg(const char *userid)
	{
		char str_query[100];
		sprintf(str_query, "select timestamp, userfrom, userto, text from msg where userto='%s' and status='1' limit 0, 1000;", userid);
		MYSQL_RES *res = pmysql->query(str_query);
		MYSQL_ROW row;
		msg *ret = new msg [mysql_field_count(pmysql->connect())+1];
		int i=0;
		if(res)
		{
			while( row = mysql_fetch_row(res) )
			{
				ret[i++] = {1, atoi(row[0]), row[1], row[2], row[3]};
			}
		}
		ret[i] = {0, 0, NULL, NULL, NULL};
		mysql_free_result(res);
		return ret;
	}
	bool insertNewMsg(const char *from, const char *to, const char * text)
	{
		char str_query[500];
		int timestamp = time(0);
		sprintf(str_query, "insert into msg (timestamp, userfrom, userto, text, status) values ('%d', '%s', '%s', '%s', '1');", timestamp, from, to, text);
		MYSQL_RES *res = pmysql->query(str_query);
		if(0 < mysql_affected_rows(pmysql->connect()))
		{
			return true;
		}
		return false;
	}
};



main()
{
	UserAction ua("172.12.72.74", "root", "123", "chat_broadcast", 3306);
	MsgAction ma("172.12.72.74", "root", "123", "chat_broadcast", 3306);

	cout <<ua.login("1024", "123")<<endl;
	cout <<ua.login("2048", "123")<<endl;
	cout <<ma.insertNewMsg("1024", "*", "this is a test for insertion")<<endl;
	msg* ret = ma.getLatestMsg("*");
	for(int i=0; ret[i].userfrom; i++)
	{
		cout <<ret[i].userfrom<<" "<<ret[i].timestamp<<":\n<--- "<<ret[i].text<<endl;
	}
}




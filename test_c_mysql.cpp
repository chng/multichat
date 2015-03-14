#include <mysql.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>


using namespace std;

/* Model */
struct user
{
	char *userid;
	char *username;
	char *key;
};

struct msg
{
	bool status;
	char *timestamp;
	char *from;
	char *to;
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
		if(!conn)
		{
			conn = mysql_real_connect(conn, dbhost, username, key, dbname, port, NULL, 0);
		}
		return conn;
	}
	
	void close()
	{
		if(conn)
			mysql_close(conn);
	}
	
	MYSQL_RES * query(const char *str_query)
	{
		if(!conn)
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
		sprintf(str_query, "select COUNT(*) from user where userid='%s' and key='%s'", userid, key);
		MYSQL_RES *res = pmysql->query(str_query);
		
		if(res)
		{
			MYSQL_ROW row = mysql_fetch_row(res);
			if(atoi(row[0])) return true;
		}
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
		sprintf(str_query, "select timestamp, from, to, text from msg where to='%s' and status='1' limit 0, 1000", userid);
		MYSQL_RES *res = pmysql->query(str_query);
		MYSQL_ROW row;
		msg *ret = new msg [mysql_field_count(pmysql->connect())+1];
		int i=0;
		if(res)
		{
			while( row = mysql_fetch_row(res) )
			{
				ret[i++] = {1, row[0], row[1], row[2], row[3]};
			}
		}
		ret[i] = {0, NULL, NULL, NULL, NULL};
		return ret;
	}
};



main()
{
	UserAction ua("172.12.72.74", "root", "", "chat_broadcast", 0);
	//MsgAction ma;
	cout <<ua.login("1024", "123")<<endl;
	cout <<ua.login("2048", "123")<<endl;
}




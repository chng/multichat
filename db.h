#ifndef __MYSQL_H
#define __MYSQL_H
#include <mysql.h>
#endif
#ifndef __STRING_H
#define __STRING_H
#include <string.h>
#endif
#ifndef __STDIO_H
#define __STDIO_H
#include <stdio.h>
#endif
#ifndef __STDLIB_H
#define __STDLIB_H
#include <stdlib.h>
#endif
#ifndef __TIME_H
#define __TIME_H
#include <time.h>
#endif
#ifndef __IOSTREAM_H
#define __IOSTREAM_H
#include <iostream>
using namespace std;
#endif


#ifndef DEEP_COPY_STR
#define DEEP_COPY_STR(to, from) if(!from) to = new char [strlen(from)+1], strcpy(to, from);
#endif

/* Model */
#ifndef MODEL_USER
#define MODEL_USER
struct user
{
	char *userid;
	char *username;
	char *password;

	user(char *_userid=NULL, char *_username=NULL, char *_password=NULL)
	:userid(_userid), username(_username), password(_password) 
	{
		//DEEP_COPY_STR(userid, _userid)
		//DEEP_COPY_STR(username, _username)
		//DEEP_COPY_STR(password, _password)
		//if(_userid) userid = new char [strlen(_userid)+1], strcpy(userid, _userid);
		//if(_username) username = new char [strlen(_username)+1], strcpy(username, _username);
		//if(_password) password = new char [strlen(_password)+1], strcpy(password, _password);
	}
	/*
	user(const user& u)
	{
		//DEEP_COPY_STR(userid, u.userid)
		//DEEP_COPY_STR(username, u.username)
		//DEEP_COPY_STR(password, u.password)
		if(u.userid) userid = new char [strlen(u.userid)+1], strcpy(userid, u.userid);
		if(u.username) username = new char [strlen(u.username)+1], strcpy(username, u.username);
		if(u.password) password = new char [strlen(u.password)+1], strcpy(password, u.password);
	}

	~user()
	{
		if(userid) delete userid;
		if(username) delete username;
		if(password) delete password;
	}
	*/
};
#endif

#ifndef MODEL_MSG
#define MODEL_MSG
struct msg
{
	int status;
	int  timestamp;
	char *userfrom;
	char *userto;
	char *text;

	msg(int _status=0, int _ts=0, char *_userfrom=NULL, char *_userto=NULL, char *_text=NULL)
	:status(_status), timestamp(_ts), userfrom(_userfrom), userto(_userto), text(_text)
	{
		//DEEP_COPY_STR(userfrom, _userfrom)
		//DEEP_COPY_STR(userto, _userto)
		//DEEP_COPY_STR(text, _text)
		//if(_userfrom) userfrom = new char [strlen(_userfrom)+1], strcpy(userfrom, _userfrom);
		//if(_userto) userto = new char [strlen(_userto)+1], strcpy(userto, _userto);
		//if(_text) text = new char [strlen(_text)+1], strcpy(text, _text);
	}
	/*
	msg(const msg& m)
	{
		timestamp = m.timestamp;
		status = m.status;
		//DEEP_COPY_STR(userfrom, m.userfrom)
		//DEEP_COPY_STR(userto, m.userto)
		//DEEP_COPY_STR(text, m.text)
		if(m.userfrom) userfrom = new char [strlen(m.userfrom)+1], strcpy(userfrom, m.userfrom);
		if(m.userto) userto = new char [strlen(m.userto)+1], strcpy(userto, m.userto);
		if(m.text) text = new char [strlen(m.text)+1], strcpy(text, m.text);
	}

	~msg()
	{
		if(userfrom) delete userfrom;
		if(userto) delete userto;
		if(text) delete text;
	}*/
};
#endif



/* DAL */
#ifndef DAL_CMYSQL
#define DAL_CMYSQL
class CMYSQL
{
	MYSQL *conn;
	char *dbhost;
	char *username;
	char *key;
	char *dbname;
	unsigned int port;
	bool isconn;
	static CMYSQL *pmysql;

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

	bool update(const char *str_update)
	{
		if(!isconn)
			return 0;
		mysql_query(conn, str_update);
		return mysql_affected_rows(conn);
	}
};
CMYSQL * CMYSQL::pmysql = NULL;

#endif

/* BLL */

#ifndef MODEL_MSG_NAME
#define MODEL_MSG_NAME
struct msg_name: public msg
{
	char *userfrom_name;
	
	msg_name(long _ts=0, int _status=0, char *_userfrom_name=NULL, char *_userfrom=NULL, char *_userto=NULL, char *_text=NULL )
	:userfrom_name(_userfrom_name), msg(_status, _ts, _userfrom, _userto, _text)
	{
		//DEEP_COPY_STR(userfrom_name, _userfrom_name);
		//if(_userfrom_name) userfrom_name = new char [strlen(_userfrom_name)+1], strcpy(userfrom_name, _userfrom_name);
	}

	msg_name(const msg_name &m):userfrom_name(m.userfrom_name), msg(m.status, m.timestamp, m.userfrom, m.userto, m.text)
	{
		//DEEP_COPY_STR(userfrom_name, m.userfrom_name);
		//if(m.userfrom_name) userfrom_name = new char [strlen(m.userfrom_name)+1], strcpy(userfrom_name, m.userfrom_name);
	}
	/*
	msg_name & operator = (const msg_name & m)
	{
		status = m.status;
		timestamp = m.timestamp;
		if(m.userfrom_name) userfrom_name = new char [strlen(m.userfrom_name)+1], strcpy(userfrom_name, m.userfrom_name);
		if(m.userfrom) userfrom = new char [strlen(m.userfrom)+1], strcpy(userfrom, m.userfrom);
		if(m.userto) userto = new char [strlen(m.userto)+1], strcpy(userto, m.userto);
		if(m.text) text = new char [strlen(m.text)+1], strcpy(text, m.text);
	}

	msg_name & copy(long _ts=0, int _status=0, char *_userfrom_name=NULL, char *_userfrom=NULL, char *_userto=NULL, char *_text=NULL )
	{
		status = _status;
		timestamp = _ts;
		if(_userfro_na_) userfro_na_ = new char [strlen(_userfro_na_)+1], strcpy(userfro_na_, _userfro_na_);
		if(_userfro_ userfro_= new char [strlen(_userfro_+1], strcpy(userfro_ _userfro_;
		if(_userto) userto = new char [strlen(_userto)+1], strcpy(userto, _userto);
		if(_text) text = new char [strlen(_text)+1], strcpy(text, _text);
	}
	~msg_name()
	{
		if(userfrom_name) delete userfrom_name;
	}
	*/

};
#endif

#ifndef BLL_USERACTION
#define BLL_USERACTION
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
		if(!res) return false;
		MYSQL_ROW row = mysql_fetch_row(res);
		mysql_free_result(res);
		if(atoi(row[0])) return true;
	}
	
	unsigned int getUserCount()
	{
		MYSQL_RES *res = pmysql->query("select COUNT(*) from user;");
		if(!res) return 0;
		MYSQL_ROW row = mysql_fetch_row(res);
		mysql_free_result(res);
		return atoi(row[0]);
	}
};
#endif

#ifndef BLL_MSGACTION
#define BLL_MSGACTION
class MsgAction
{
	MYSQL_RES *res;
	CMYSQL * pmysql;
public:
	MsgAction(const char *dbhost, const char *username, const char *key, const char *dbname, unsigned int port)
	{
		res = NULL;
		pmysql = CMYSQL::getInstance(dbhost, username, key, dbname, port);
		pmysql->connect();
	}
	~MsgAction()
	{
		pmysql->close();
	}

	const msg_name *getLatestMsg(const char *userquery, const char *userto)
	{
		char str_query[500];
		sprintf(str_query, "select timestamp, status, user.username, userfrom, userto, text from user, msg where msg.userfrom=user.userid and msg.userto like '%s' and msg.status<>0 limit 0, 1000;", userto);
		res = pmysql->query(str_query);
		if(res)
		{
			const unsigned rnum = mysql_num_rows(res);
			msg_name *ret = new msg_name [rnum+1];
			int i = rnum;	
			MYSQL_ROW row;
			while(i--)
			{
				row = mysql_fetch_row(res);
				ret[i] = msg_name(atoi(row[0]), atoi(row[1]), row[2], row[3], row[4], row[5]);
				//ret[i] = tmp;
			}
			//mysql_free_result(res);
			i = rnum;
			while(i--)
			{
				sprintf(str_query, "update msg set status=%d where status<>0 and timestamp='%d' and userfrom='%s';", ret[i].status-1, ret[i].timestamp, ret[i].userfrom);
				pmysql->update(str_query);
			}
			return ret;
		}
		return NULL;
	}

	void freeResult()
	{
		if(res) mysql_free_result(res), res = 0;
	}

	bool insertNewMsg(const char *from, const char *to, const char * text)
	{
		char str_query[500];
		int timestamp = time(0);
		sprintf(str_query, "insert into msg (timestamp, userfrom, userto, text, status) values ('%d', '%s', '%s', '%s', '1');", timestamp, from, to, text);
		if( pmysql->update(str_query) && 0 < mysql_affected_rows(pmysql->connect()))
		{
			return true;
		}
		return false;
	}
};
#endif

/*
main()
{
	UserAction ua("172.12.72.74", "root", "123", "chat_broadcast", 3306);
	MsgAction ma("172.12.72.74", "root", "123", "chat_broadcast", 3306);
	cout <<ua.login("1024", "123")<<endl;
	cout <<ua.login("2048", "123")<<endl;
	cout <<ma.insertNewMsg("1024", "*", "this is a test for insertion")<<endl;
	const msg_name *ret = ma.getLatestMsg("1024", "%");
	for(int i=0; ret[i].userfrom; i++)
	{
		cout <<ret[i].userfrom<<" "<<ret[i].timestamp<<":\n<--- "<<ret[i].text<<endl;
	}
}
*/

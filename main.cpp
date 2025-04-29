#include <iostream>
#include "Connection.h"
#include "CommonConnectionPool.h"


int main()
{
	/*
	Connection conn;
	char sql[1024] = { 0 };
	sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
		"zhang san", 20, "male");
	bool bConn = conn.connect("127.0.0.1", 3307, "root", "", "chat");
	conn.update(sql);
	*/

	ConnectionPool* cp = ConnectionPool::getConnectionPool();
	//cp->loadConfigFile();

	clock_t begin = clock();

	std::thread t1([]() {
		ConnectionPool* cp = ConnectionPool::getConnectionPool();
		for (int i = 0; i < 250; ++i)
		{
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");

			/*
			Connection conn;
			char sql[1024] = { 0 };
			bool bConn = conn.connect("127.0.0.1", 3307, "root", "", "chat");
			conn.update(sql);
			*/
			auto sp = cp->getConnection();
			sp->update(sql);
		}
		});

	std::thread t2([]() {
		ConnectionPool* cp = ConnectionPool::getConnectionPool();
		for (int i = 0; i < 250; ++i)
		{
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");

			/*
			Connection conn;
			char sql[1024] = { 0 };
			bool bConn = conn.connect("127.0.0.1", 3307, "root", "", "chat");
			conn.update(sql);
			*/
			//ConnectionPool* cp = ConnectionPool::getConnectionPool();
			auto sp = cp->getConnection();
			sp->update(sql);
		}
		});

	std::thread t3([]() {
		ConnectionPool* cp = ConnectionPool::getConnectionPool();
		for (int i = 0; i < 250; ++i)
		{
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");

			/*
			Connection conn;
			char sql[1024] = { 0 };
			bool bConn = conn.connect("127.0.0.1", 3307, "root", "", "chat");
			conn.update(sql);
			*/
			//ConnectionPool* cp = ConnectionPool::getConnectionPool();
			auto sp = cp->getConnection();
			sp->update(sql);
		}
		});

	std::thread t4([]() {
		ConnectionPool* cp = ConnectionPool::getConnectionPool();
		for (int i = 0; i < 250; ++i)
		{
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");

			/*
			Connection conn;
			char sql[1024] = { 0 };
			bool bConn = conn.connect("127.0.0.1", 3307, "root", "", "chat");
			conn.update(sql);
			*/
			//ConnectionPool* cp = ConnectionPool::getConnectionPool();
			auto sp = cp->getConnection();
			sp->update(sql);
		}
		});

	t1.join();
	t2.join();
	t3.join();
	t4.join();
	clock_t end = clock();
	std::cout << "花费时间：" << end - begin << "ms" << std::endl;


#if 0
	for (int i = 0; i < 1000; ++i)
	{
		char sql[1024] = { 0 };
		sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
			"zhang san", 20, "male");

		/*
		Connection conn;
		char sql[1024] = { 0 };
		bool bConn = conn.connect("127.0.0.1", 3307, "root", "", "chat");
		conn.update(sql);
		*/
		ConnectionPool* cp = ConnectionPool::getConnectionPool();
		auto sp = cp->getConnection();
		sp->update(sql);
	}
#endif

	return 0;
}
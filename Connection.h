#pragma once
#include <mysql.h>
#include <string>
#include <ctime>
#include "public.h"

/*
ʵ��MySQL���ݿ�Ĳ���
*/
class Connection
{
public:
	// ��ʼ����������
	Connection();

	// �ͷ����ݿ�������Դ
	~Connection();

	// �������ݿ�
	bool connect(std::string ip, unsigned short port, std::string user,
		std::string password, std::string dbname);

	// ���²��� insert��delete��update
	bool update(std::string sql);

	// ��ѯ���� select
	MYSQL_RES* query(std::string sql);

	// ����һ�����ӵ���ʼ�Ŀ���ʱ���
	void refreshAliveTime() { _alivetime = clock(); }
	// ���ش���ʱ��
	clock_t getAliceTime() const { return clock() - _alivetime; }
private:
	MYSQL* _conn;			// ��ʾ��MySQL Server��һ������
	clock_t _alivetime;		//��¼�������״̬�����ʼ���ʱ��
};
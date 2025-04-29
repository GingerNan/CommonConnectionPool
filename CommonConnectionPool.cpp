#include "CommonConnectionPool.h"
#include "public.h"

// �̰߳�ȫ���������������ӿ�
ConnectionPool* ConnectionPool::getConnectionPool()
{
    static ConnectionPool pool; // �������Զ�����lock��unlock
    return &pool;
}

ConnectionPool::ConnectionPool()
{
    // ����������
    if (!loadConfigFile())
    {
        return;
    }

    // ������ʼ����������
    for (int i = 0; i < _initSize; ++i)
    {
        Connection* p = new Connection();
        if (!p->connect(_ip, _port, _username, _password, _dbname))
        {
            LOG("MySql connect Error!");
            return;
        }
        p->refreshAliveTime();  // ˢ��һ�¿�ʼ���е���ʼʱ��
        _connectionQue.push(p);
        _connectionCnt++;
    }

    // ����һ���µ��̣߳���Ϊ���ӵ�������
    std::thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
    produce.detach();

    // ����һ���µĶ�ʱ�̣߳�ɨ�賬��maxIdleTimeʱ��Ŀ������ӣ����ж�������ӻ���
    std::thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
    scanner.detach();
}

bool ConnectionPool::loadConfigFile()
{
    FILE* pf = fopen("mysql.ini", "r");
    if (pf == nullptr)
    {
        LOG("mysql.ini file is not exist!");
        return false;
    }

    while (!feof(pf))
    {
        char line[1024] = { 0 };
        fgets(line, 1024, pf);
        std::string str = line;
        size_t idx = str.find('=', 0);
        if (idx == -1) //��Ч��������
        {
            continue;
        }

        // password=123456\n
        size_t endidex = str.find('\n', idx);
        std::string key = str.substr(0, idx);
        std::string val = str.substr(idx + 1, endidex - idx - 1);

        if (key == "ip")
            _ip = val;
        else if (key == "port")
            _port = atoi(val.c_str());
        else if (key == "username")
            _username = val;
        else if (key == "password")
            _password = val;
        else if (key == "dbname")
            _dbname = val;
        else if (key == "maxSize")
            _maxSize = atoi(val.c_str());
        else if (key == "maxIdleTime")
            _maxIdleTime = atoi(val.c_str());
        else if (key == "connectionTimeOut")
            _connectionTimeout = atoi(val.c_str());
    }
    return true;
}

void ConnectionPool::produceConnectionTask()
{
    for (;;)
    {
        std::unique_lock<std::mutex> lock(_queueMutex);
        while (!_connectionQue.empty())
        {
            _cv.wait(lock); // ���в��գ��˴������߳̽���ȴ�״̬
        }

        // ��������û�е������ޣ����������µ�����
        if (_connectionCnt < _maxSize)
        {
            Connection* p = new Connection();
            if (!p->connect(_ip, _port, _username, _password, _dbname))
            {
                LOG("MySql connect Error!");
                return;
            }
            p->refreshAliveTime();  // ˢ��һ�¿�ʼ���е���ʼʱ��
            _connectionQue.push(p);
            _connectionCnt++;
        }

        // ֪ͨ�������̣߳���������������
        _cv.notify_all();
    }
}

void ConnectionPool::scannerConnectionTask()
{
    for (;;)
    {
        // ͨ��sleepģ�ⶨʱЧ��
        std::this_thread::sleep_for(std::chrono::seconds(_maxIdleTime));

        // ɨ���������У��ͷŶ��������
        std::unique_lock<std::mutex> lock(_queueMutex);
        while (_connectionCnt > _initSize)
        {
            Connection* p = _connectionQue.front();
            if (p->getAliceTime() >= (_maxIdleTime * 1000))
            {
                _connectionQue.pop();
                _connectionCnt--;
                delete p;   // ����~Connection() �ͷ�����
            }
            else
            {
                break; // ��ͷ������û�г���_maxIdleTime���������ӿ϶�û��
            }
        }
    }
}

std::shared_ptr<Connection> ConnectionPool::getConnection()
{
    std::unique_lock<std::mutex> lock(_queueMutex);
    while (_connectionQue.empty())
    {
        // sleep
        if (std::cv_status::timeout == _cv.wait_for(lock, std::chrono::milliseconds(_maxIdleTime)))
        {
            if (_connectionQue.empty())
            {
                LOG("��ȡ�������ӳ�ʱ...��ȡ����ʧ�ܣ�");
                return nullptr;
            }
        }
    }

    /*
    shared_ptr����ָ������ʱ�����connection��Դֱ��delete�����൱��
    ����connection������������connection�ͱ�close���ˡ�
    ������Ҫ�Զ���shared_ptr���ͷ���Դ�ķ�ʽ����connectionֱ�ӹ黹��queue����
    */
    std::shared_ptr<Connection> sp(_connectionQue.front(),
        [&](Connection* pcon) 
        {
            // �������ڷ�����Ӧ���߳��е��õģ�����һ��Ҫ���Ƕ��е��̰߳�ȫ����
            std::unique_lock<std::mutex> lock(_queueMutex);
            pcon->refreshAliveTime();  // ˢ��һ�¿�ʼ���е���ʼʱ��
            _connectionQue.push(pcon);
        });
    _connectionQue.pop();
    _cv.notify_all();   // �����������Ժ�֪ͨ�������̼߳��һ�£��������Ϊ���ˣ��Ͻ���������

    return sp;
}
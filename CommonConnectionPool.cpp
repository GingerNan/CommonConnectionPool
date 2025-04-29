#include "CommonConnectionPool.h"
#include "public.h"

// 线程安全的懒汉单例函数接口
ConnectionPool* ConnectionPool::getConnectionPool()
{
    static ConnectionPool pool; // 编译器自动进行lock和unlock
    return &pool;
}

ConnectionPool::ConnectionPool()
{
    // 加载配置项
    if (!loadConfigFile())
    {
        return;
    }

    // 创建初始数量的连接
    for (int i = 0; i < _initSize; ++i)
    {
        Connection* p = new Connection();
        if (!p->connect(_ip, _port, _username, _password, _dbname))
        {
            LOG("MySql connect Error!");
            return;
        }
        p->refreshAliveTime();  // 刷新一下开始空闲的起始时间
        _connectionQue.push(p);
        _connectionCnt++;
    }

    // 启动一个新的线程，作为连接的生产者
    std::thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
    produce.detach();

    // 启动一个新的定时线程，扫描超过maxIdleTime时间的空闲连接，进行多余的连接回收
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
        if (idx == -1) //无效的配置项
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
            _cv.wait(lock); // 队列不空，此处生产线程进入等待状态
        }

        // 连接数量没有到达上限，继续创建新的连接
        if (_connectionCnt < _maxSize)
        {
            Connection* p = new Connection();
            if (!p->connect(_ip, _port, _username, _password, _dbname))
            {
                LOG("MySql connect Error!");
                return;
            }
            p->refreshAliveTime();  // 刷新一下开始空闲的起始时间
            _connectionQue.push(p);
            _connectionCnt++;
        }

        // 通知消费者线程，可以消费连接了
        _cv.notify_all();
    }
}

void ConnectionPool::scannerConnectionTask()
{
    for (;;)
    {
        // 通过sleep模拟定时效果
        std::this_thread::sleep_for(std::chrono::seconds(_maxIdleTime));

        // 扫描整个队列，释放多余的连接
        std::unique_lock<std::mutex> lock(_queueMutex);
        while (_connectionCnt > _initSize)
        {
            Connection* p = _connectionQue.front();
            if (p->getAliceTime() >= (_maxIdleTime * 1000))
            {
                _connectionQue.pop();
                _connectionCnt--;
                delete p;   // 调用~Connection() 释放连接
            }
            else
            {
                break; // 队头的连接没有超过_maxIdleTime，其他连接肯定没有
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
                LOG("获取空闲连接超时...获取连接失败！");
                return nullptr;
            }
        }
    }

    /*
    shared_ptr智能指针析构时，会把connection资源直接delete掉，相当于
    调用connection的析构函数，connection就被close掉了。
    这里需要自定义shared_ptr的释放资源的方式，把connection直接归还到queue当中
    */
    std::shared_ptr<Connection> sp(_connectionQue.front(),
        [&](Connection* pcon) 
        {
            // 这里是在服务器应用线程中调用的，所以一定要考虑队列的线程安全操作
            std::unique_lock<std::mutex> lock(_queueMutex);
            pcon->refreshAliveTime();  // 刷新一下开始空闲的起始时间
            _connectionQue.push(pcon);
        });
    _connectionQue.pop();
    _cv.notify_all();   // 消费完连接以后，通知生产者线程检查一下，如果队列为空了，赶紧生产连接

    return sp;
}
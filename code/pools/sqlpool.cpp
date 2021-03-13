#include "sqlpool.h"

sqlPool::sqlPool()
{
    userConn_ = 0;
    freeConn_ = 0;
}

sqlPool *sqlPool::getInstance()
{
    static sqlPool sqlPool;
    return &sqlPool;
}

void sqlPool::Init(const char *host, int port, const char *userName, const char *passwd, const char *dbName, int connSize)
{
    assert(connSize > 0);
    for (int i = 0; i < connSize; i++)
    {
        MYSQL *sql = nullptr;
        //初始化连接
        sql = mysql_init(sql);
        assert(sql);

        //简历mysql数据库连接
        sql = mysql_real_connect(sql, host, userName, passwd, dbName, port, nullptr, 0);
        if (!sql)
        {
            std::cout << "MySql Connect error!" << std::endl;
        }
        connQue_.push(sql);
    }
    MAX_CONN_ = connSize;
    sem_init(&semId_, 0, MAX_CONN_);
}

MYSQL *sqlPool::getConn()
{
    MYSQL *sql = nullptr;
    if (connQue_.empty())
        return sql;
    sem_wait(&semId_);
    //在下面的作用域中，使用lock_guard将mtx_转换为智能锁，声明时加锁，析构时解锁
    {
        std::lock_guard<std::mutex> locker(mtx_);
        sql = connQue_.front();
        connQue_.pop();
    }
    return sql;
}

void sqlPool::freeConn(MYSQL *sql)
{
    assert(sql);
    std::lock_guard<std::mutex> locker(mtx_);
    connQue_.push(sql);
    sem_post(&semId_);
}
//关闭时还有用户的问题
void sqlPool::closePool()
{
    std::lock_guard<std::mutex> locker(mtx_);
    while (!connQue_.empty())
    {
        auto item = connQue_.front();
        connQue_.pop();
        mysql_close(item);
    }
    //中止mysql服务
    mysql_library_end();
}

int sqlPool::getFreeConnCount()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return connQue_.size();
}

sqlPool::~sqlPool()
{
    closePool();
}
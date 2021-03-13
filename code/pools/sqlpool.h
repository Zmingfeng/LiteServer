#ifndef SQLPOOL_H
#define SQLPOOL_H
#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include <iostream>
#include <assert.h>
class sqlPool
{
public:
    static sqlPool *getInstance();

    MYSQL *getConn();
    int getFreeConnCount();
    void freeConn(MYSQL *sql);
    void Init(const char *host, int port, const char *userName, const char *passwd, const char *dbName, int connSize);
    void closePool();

private:
    sqlPool();
    ~sqlPool();
    int MAX_CONN_;
    //这两个变量暂时不知道作用
    int userConn_;
    int freeConn_;

    std::queue<MYSQL *> connQue_;
    std::mutex mtx_;
    sem_t semId_;
};

#endif
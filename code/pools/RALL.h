#ifndef RALL_H
#define RALL_H
#include <mysql/mysql.h>
#include "sqlpool.h"
class sqlConnRAII
{
public:
    sqlConnRAII(MYSQL **sql, sqlPool *connPool)
    {
        assert(connPool);
        *sql = connPool->getConn();
        sql_ = *sql;
        connPool_ = connPool;
    }

    ~sqlConnRAII()
    {
        if (sql_)
        {
            connPool_->freeConn(sql_);
        }
    }

private:
    MYSQL *sql_;
    sqlPool *connPool_;
};
#endif
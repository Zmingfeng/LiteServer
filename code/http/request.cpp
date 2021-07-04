#include "request.h"

const std::unordered_set<std::string> httpRequest::DEFAULT_HTML = {
    "/index",
    "/register",
    "/login",
    "/welcome",
    "/video",
    "/picture",
};
const std::unordered_map<std::string, int> httpRequest::DEFAULT_HTML_FLAG = {
    {"/register.html", 0},
    {"/login.html", 1},
};
void httpRequest::Init()
{
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    headers_.clear();
    posts_.clear();
}

bool httpRequest::isKeepAlive() const
{
    if (headers_.count("Connection") == 1)
    {
        return headers_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool httpRequest::parse(Buffer &readBuffer)
{
    const char CRLF[] = "\r\n"; //两个字节
    if (readBuffer.getReadableBytes() <= 0)
    {
        return false;
    }
    while (readBuffer.getReadableBytes() && state_ != FINISH)
    {
        //返回第一个/r/n中指向/r的指针
        const char *lineEnd = std::search(readBuffer.getReadPtrConst(), readBuffer.getWritePtrConst(), CRLF, CRLF + 2);

        std::string line(readBuffer.getReadPtrConst(), lineEnd);

        switch (state_)
        {
        case REQUEST_LINE:
            if (!parseRequestLine_(line))
            {
                return false;
            }
            state_ = HEADERS;
            parsePath_();
            break;
        case HEADERS:
            //分析完请求行没问题的话，一般头部也不会出问题，所以这里的处理比较简陋，只有请求头读完了才进行状态转移
            if (parseHeaders_(line))
                state_ = HEADERS;
            else
                state_ = BODY;
            if (readBuffer.getReadableBytes() <= 2)
            {
                state_ = FINISH;
            }
            break;
        case BODY:
            parseBody_(line);
            state_ = FINISH;
            break;
        default:
            break;
        }

        if (lineEnd == readBuffer.getWritePtrConst())
            break;
        readBuffer.addReadPos(lineEnd - readBuffer.getReadPtrConst() + 2);
    }
    return true;
}
bool httpRequest::parseBody_(const std::string &line)
{
    body_ = line;
    parsePost_();
}
bool httpRequest::parsePost_()
{
    //"application/x-www-form-urlencoded" 表示 form表单数据被编码为key/value格式发送到服务器
    if (method_ == "POST" && headers_["Content-Type"] == "application/x-www-form-urlencoded")
    {
        parseFromUrlencoded_();
        if (DEFAULT_HTML_FLAG.count(path_))
        {
            int tag = DEFAULT_HTML_FLAG.find(path_)->second;
            if (tag == 0 || tag == 1)
            {
                bool isLogin = (tag == 1);
                if (userVerify(posts_["username"], posts_["password"], isLogin))
                {
                    path_ = "welcome.html";
                    return true;
                }
                else
                {
                    path_ = "error.html";
                }
            }
        }
    }
    return false;
}
bool httpRequest::userVerify(const std::string username, const std::string passwd, bool isLogin)
{
    if (username == "" || passwd == "")
        return false;
    MYSQL *sql;
    sqlConnRAII s(&sql, sqlPool::getInstance());
    assert(sql);
    char cmd[256] = {0};

    snprintf(cmd, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", username.c_str());
    MYSQL_RES *res = nullptr;
    MYSQL_FIELD *fields = nullptr;
    int numsOfFields = 0;
    if (mysql_query(sql, cmd) != 0)
    {
        sqlPool::getInstance()->freeConn(sql);
        return false;
    }
    res = mysql_store_result(sql);
    numsOfFields = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    while (MYSQL_ROW row = mysql_fetch_row(res))
    {
        //注册时有同名用户
        if (!isLogin)
        {
            mysql_free_result(res);
            sqlPool::getInstance()->freeConn(sql);
            return false;
        }
        std::string pwd(row[1]);
        //登录行为， 且查询到了用户
        if (isLogin)
        {
            bool isExist = false;
            if (pwd == passwd)
            {
                isExist = true;
            }
            mysql_free_result(res);
            sqlPool::getInstance()->freeConn(sql);
            return isExist;
        }
    }

    if (!isLogin)
    {
        std::cout << "resigiter!" << std::endl;
        bzero(cmd, 256);
        snprintf(cmd, 256, "INSERT INTO user(username, pasword) VALUES('%s', '%s')", username.c_str(), passwd.c_str());
        if (mysql_query(sql, cmd) != 0)
        {
            sqlPool::getInstance()->freeConn(sql);
            return false;
        }
        sqlPool::getInstance()->freeConn(sql);
        return true;
    }
    return false;
}
int ConverHex(const char ch)
{
    if (ch >= 'A' && ch <= 'F')
    {
        return ch - 'A' + 10;
    }
    else if (ch >= 'a' && ch <= 'f')
    {
        return ch - 'a' + 10;
    }
    else
        return ch - '0';
}
void httpRequest::parseFromUrlencoded_()
{
    if (body_.size() == 0)
    {
        return;
    }

    std::string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for (; i < n; i++)
    {
        char ch = body_[i];
        switch (ch)
        {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            posts_[key] = value;
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if (posts_.count(key) == 0 && j < i)
    {
        value = body_.substr(j, i - j);
        posts_[key] = value;
    }
}
bool httpRequest::parseHeaders_(const std::string &line)
{
    std::regex pattern("^([^ ]*): ?(.*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, pattern))
    {
        headers_[subMatch[1]] = subMatch[2]; //这种用法没咋见过
        return true;
    }
    return false;
}
void httpRequest::parsePath_()
{
    if (path_ == "/")
    {
        path_ = "index.html";
    }
    else
    {
        for (auto &item : DEFAULT_HTML) //如果请求的是图片，这里就没有，所以这里很简陋，可以优化
        {
            if (item == path_)
            {
                path_ += ".html";
                break;
            }
        }
    }
}
bool httpRequest::parseRequestLine_(const std::string &line)
{
    std::regex pattern("^([^ ]*) ^([^ ]*) ^([^ ]*)");

    std::smatch subMatch;
    if (std::regex_match(line, subMatch, pattern))
    {
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        return true;
    }
    return false;
}
std::string &httpRequest ::getPath()
{
    return path_;
}
std::string httpRequest::getPath() const
{
    return path_;
}
std::string httpRequest::getVersion() const
{
    return version_;
}
std::string httpRequest::getMethod() const
{
    return method_;
}
std::string httpRequest::getPost(const std::string &key) const
{
    assert(key != "");
    if (posts_.count(key) == 1)
    {
        return posts_.find(key)->second;
    }
    return "";
}

std::string httpRequest::getPost(const char *key) const
{
    assert(key);
    if (posts_.count(key) == 1)
    {
        return posts_.find(key)->second;
    }
    return "";
}

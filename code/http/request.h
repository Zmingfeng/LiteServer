#ifndef REQUEST_H
#define REQUEST_H
#include <string>
#include <regex>
#include <errno.h>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <mysql/mysql.h>
#include "../pools/RALL.h"
#include "../pools/sqlpool.h"
#include "../buffer/buffer.h"
class httpRequest
{

public:
    enum PARSE_STATE
    {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };
    enum HTTP_CODE
    {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    httpRequest() { Init(); };
    ~httpRequest() = default;
    void Init();
    bool parse(Buffer &buff);
    std::string getPost(const std::string &key) const;
    std::string getPost(const char *key) const;
    std::string &getPath();
    std::string getPath() const;
    std::string getVersion() const;
    std::string getMethod() const;

    std::string path() const;
    std::string &path();
    std::string method() const;

    bool isKeepAlive() const;

private:
    bool parseRequestLine_(const std::string &line);
    bool parseHeaders_(const std::string &line);
    bool parseBody_(const std::string &line);
    bool parsePost_();
    void parseFromUrlencoded_();
    void parsePath_();
    bool userVerify(const std::string username, const std::string passwd, bool isLogin);
    std::string method_, path_, version_, body_;
    PARSE_STATE state_;

    std::unordered_map<std::string, std::string> headers_;
    std::unordered_map<std::string, std::string> posts_;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_FLAG;
};
#endif
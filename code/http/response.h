#ifndef RESPONSE_H
#define RESPONSE_H
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <unordered_map>
#include <string.h>
#include <cstring>
#include "../buffer/buffer.h"
class httpResponse
{
public:
    httpResponse();
    ~httpResponse();

    void Init(const std::string &rootDir, std::string &filePath, bool isKeepAlive = false, int code = -1);
    void unMapfile();
    void makeResponse(Buffer &buff);
    char *getFile();
    size_t getFileLength() const;

private:
    int code_;
    bool isKeepAlive_;
    char *mmFile_;
    struct stat mmFileStat_;
    static std::string path_;
    static std::string rootDir_;
    static const std::unordered_map<int, std::string> CODE_PATH;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    void errorHtml_();
    void addStateLine_(Buffer &buff);
    void addHeader_(Buffer &buff);
    void addContent_(Buffer &buff);
    std::string getFileType_();
    void addErrorContent_(Buffer &buff, std::string message);
};
#endif
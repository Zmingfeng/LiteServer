#include "response.h"

std::string httpResponse::path_;
std::string httpResponse::rootDir_;
const std::unordered_map<int, std::string> httpResponse::CODE_PATH = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};
const std::unordered_map<int, std::string> httpResponse::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Nodt Found"},
};
const std::unordered_map<std::string, std::string> httpResponse::SUFFIX_TYPE = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css "},
    {".js", "text/javascript "},
};
httpResponse::httpResponse()
{
    code_ = -1;
    path_ = rootDir_ = "";
    isKeepAlive_ = false;
    mmFile_ = nullptr;
    mmFileStat_ = {0};
}

httpResponse::~httpResponse()
{
    unMapfile();
}

void httpResponse::Init(const std::string &rootDir, std::string &filePath, bool isKeepAlive, int code)
{
    assert(rootDir != "");
    unMapfile();
    code_ = code;
    path_ = filePath;
    isKeepAlive_ = isKeepAlive;
    rootDir_ = rootDir;
    mmFile_ = NULL;
    mmFileStat_ = {0};
}

void httpResponse::unMapfile()
{
    if (mmFile_)
    {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}

void httpResponse::makeResponse(Buffer &buff)
{
    //获取路径中的文件属性，并放入mmFileStat_中
    if (stat((rootDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode))
    {
        code_ = 404; //页面找不到
    }
    //文件是否允许被读取
    else if (!(mmFileStat_.st_mode & S_IROTH))
    {
        code_ = 403; //权限出错
    }
    else if (code_ == -1)
    {
        code_ = 200; //OK
    }
    //处理错误码应该返回的网页,code_ == 200的话不会执行
    errorHtml_();
    //添加状态行
    addStateLine_(buff);
    //添加消息报头
    addHeader_(buff);
    //添加空行
    //添加相应正文
    addContent_(buff);
}

char *httpResponse::getFile()
{
    return mmFile_;
}
size_t httpResponse::getFileLength() const
{
    return mmFileStat_.st_size;
}

void httpResponse::errorHtml_()
{
    if (CODE_PATH.count(code_) == 1)
    {
        path_ = CODE_PATH.find(code_)->second;
        stat((rootDir_ + path_).data(), &mmFileStat_);
    }
}

void httpResponse::addStateLine_(Buffer &buff)
{
    std::string status;
    if (CODE_STATUS.count(code_) == 0)
        code_ = 400;
    status = CODE_STATUS.find(code_)->second;
    buff.Append("HTTP/1.0 " + std::to_string(code_) + " " + status + "\r\n");
}

void httpResponse::addHeader_(Buffer &buff)
{
    buff.Append("Connection: ");
    if (isKeepAlive_)
    {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max-6, timeout=120\r\n");
    }
    else
    {
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + getFileType_() + "\r\n");
}
void httpResponse::addContent_(Buffer &buff)
{
    int srcFd = open((rootDir_ + path_).data(), O_RDONLY);
    if (srcFd < 0)
    {
        addErrorContent_(buff, "File Not Found!");
        return;
    }
    //在内存中存放文件副本
    int *mmRet = (int *)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if (*mmRet == -1)
    {
        addErrorContent_(buff, "File Not Found!");
        return;
    }
    mmFile_ = (char *)mmRet;
    close(srcFd);
}
void httpResponse::addErrorContent_(Buffer &buff, std::string message)
{
    std::string body, status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (CODE_STATUS.count(code_) == 1)
    {
        status = CODE_STATUS.find(code_)->second;
    }
    else
    {
        status = "Bad Rquest";
    }
    body += std::to_string(code_) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>LiteServer</em></body></html>";
    buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}
std::string httpResponse::getFileType_()
{
    std::string::size_type idx = path_.find_last_of('.');
    if (idx == std::string::npos)
    {
        return "text/plain";
    }
    std::string suffix = path_.substr(idx);
    if (SUFFIX_TYPE.count(suffix) == 1)
    {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}
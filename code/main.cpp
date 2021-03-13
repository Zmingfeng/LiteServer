/*
 * @Author       : zmingfeng
 * @Date         : 2021-03-11
 * @copyleft Apache 2.0
 */

#include <unistd.h>
#include "webserver/webserver.h"
/*
 * @brief         主执行程序  
 * @param argc    命令行参数个数，包括自己
 * @param argv    命令行参数list，包括自己
 * @return  
 */
int main(int argc, char *argv[])
{
    WebServer server(1316, 3, 60000 /*精度到微秒*/, false, 3306, "root", "12345678", "webserver", 12, 6);
    server.start();
    return 0;
}
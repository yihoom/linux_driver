#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
/**
 * argc:应用程序参数的个数
 * argv[]:具体的参数内容，字符串形式
 * ./keyApp <filename> 
*/

#define KEYVAL    0xF0  
#define INVALKEY  0x0

int fd;
static void sigio_signal_handler(int num)
{
    int err;
    unsigned char keyval = 0;
    err = read(fd, &keyval, sizeof(keyval));

    printf("sigio signal! keyval=%d\r\n", keyval);
}

int main(int argc, char *argv[])
{
    char *filename;
    
    int ret;
    unsigned char value;
    char writebuf;
    int flags;
    if(argc != 2)
    {
        printf("error CMD");
        return -1;
    }

    filename = argv[1];

    /*1.打开驱动*/
    fd = open(filename , O_RDWR);

    if(fd < 0)
    {
        printf("open file failed\r\n");
        return -1;
    }

    /*设置信号处理函数*/
    signal(SIGIO, sigio_signal_handler);
    
    /*设置当前进程接受SIGIO*/
    fcntl(fd, F_SETOWN, getpid());
    flags = fcntl(fd, F_GETFL);
    /*异步通知*/
    fcntl(fd, F_SETFL, flags | FASYNC);

    while(1)
    {
        sleep(2);
    }
    /*关闭驱动*/

    close(fd);

    return 0;
}

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <poll.h>
/**
 * argc:应用程序参数的个数
 * argv[]:具体的参数内容，字符串形式
 * ./keyApp <filename> 
*/

#define KEYVAL    0xF0  
#define INVALKEY  0x0

int main(int argc, char *argv[])
{
    char *filename;
    int fd;
    int ret;
    unsigned char value;
    char writebuf;
    struct timeval timeout;
    fd_set readfds;
    struct pollfd fds;

    if(argc != 2)
    {
        printf("error CMD");
        return -1;
    }

    filename = argv[1];

    /*1.打开驱动*/
    fd = open(filename , O_RDWR | O_NONBLOCK);

    if(fd < 0)
    {
        printf("open file failed\r\n");
        return -1;
    }
    

    /*2。循环读取按键指*/
    // while (1)
    // {
    //     FD_ZERO(&readfds);  /* 清除 readfds */
    //     FD_SET(fd, &readfds);   /* 将 fd 添加到 readfds 里面 */
    //     timeout.tv_sec = 1;
    //     timeout.tv_usec = 0;
    //     ret =  select(fd+1, &readfds, NULL, NULL, &timeout);
    //     switch (ret)
    //     {
    //     case 0:
    //         printf("timeout\r\n");
    //         break;
    //     case -1:
    //         printf("err\r\n");
    //         break;
    //     default:
    //         if(FD_ISSET(fd, &readfds))
    //         {
    //             ret = read(fd, &value, sizeof(value));
    //             printf("KEY Pressed, value=%d\r\n", value);
    //         }
    //         break;
    //     }
    // }


    while (1)
    {
        fds.fd = fd;
        fds.events = POLLIN;
        ret =  poll(&fds, 1, 500);
        switch (ret)
        {
        case 0:
            printf("timeout\r\n");
            break;
        case -1:
            printf("err\r\n");
            break;
        default:
            if(fds.revents | POLLIN)
            {
                ret = read(fd, &value, sizeof(value));
                printf("KEY Pressed, value=%d\r\n", value);
            }
            break;
        }
    }
    


    /*关闭驱动*/

    close(fd);

    return 0;
}

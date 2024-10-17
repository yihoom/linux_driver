#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
/**
 * argc:应用程序参数的个数
 * argv[]:具体的参数内容，字符串形式
 * ./ledApp <filename> <0:1> 0表示关灯，1表示开灯
*/

#define LEDOFF 	0  	/*关闭*/
#define LEDON 	1	/*打开*/

int main(int argc, char *argv[])
{
    char *filename;
    int fd;
    int ret;
    int stat;
    char writebuf;
    if(argc != 3)
    {
        printf("error CMD");
        return -1;
    }

    filename = argv[1];
    stat = atoi(argv[2]);

    /*1.打开驱动*/
    fd = open(filename , O_RDWR);

    if(fd < 0)
    {
        printf("open file failed\r\n");
        return -1;
    }

    /*2.写驱动*/

    if(stat == LEDON)
    {
        writebuf = LEDON;
        ret = write(fd, &writebuf, 1);
    }
    else if(stat == LEDOFF)
    {
        writebuf = LEDOFF;
        ret = write(fd, &writebuf, 1);
    }
    else
    {
        printf("3rd arg wrong\r\n");
    }

    /*关闭驱动*/

    close(fd);

    return 0;
}

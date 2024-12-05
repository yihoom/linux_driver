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
 * ./keyApp <filename> 
*/

#define KEYVAL    0xF0  
#define INVALKEY  0x0

int main(int argc, char *argv[])
{
    char *filename;
    int fd;
    int ret;
    int value;
    char writebuf;
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

    /*2。循环读取按键指*/
    while (1)
    {
        read(fd, &value, sizeof(value));
        if(value == KEYVAL)
        {
            printf("KEY Pressed, value=%d\r\n", value);
        }
    }
    


    /*关闭驱动*/

    close(fd);

    return 0;
}

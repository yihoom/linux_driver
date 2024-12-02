#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
/**
 * argc:应用程序参数的个数
 * argv[]:具体的参数内容，字符串形式
*/

#define CLOSE_CMD   (_IO(0xEF, 0x1))
#define OPED_CMD    (_IO(0xEF, 0x2))
#define SetPRD_CMD  (_IOW(0xEF, 0x3, unsigned long))

#define LEDOFF 	0  	/*关闭*/
#define LEDON 	1	/*打开*/
#define SetPRD 	2	/*打开*/
void clear_buffer(void);
int main(int argc, char *argv[])
{
    char *filename;
    int fd;
    int ret;
    int cmd;
    char cmdchar;
    char writebuf;
    unsigned char str[100];
    int prd;

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

    /*2.写驱动*/


    while (1)
    {
        printf("pls input cmd:");

        
        scanf("%d", &cmd);
        clear_buffer();
        printf("cmd=%d\r\n", cmd);
        switch (cmd)
        {
        case 0:
            ret = ioctl(fd, CLOSE_CMD);
            break;
        case 1:
            ret = ioctl(fd, OPED_CMD);
            break;
        case 2:
            printf("pls input prd:");
            scanf("%d", &prd);
            clear_buffer();
            ret = ioctl(fd, SetPRD_CMD, prd);
            break;
        default:
            break;
        }
    }
    

    /*关闭驱动*/

    close(fd);

    return 0;
}

void clear_buffer(void)
{
    int c;
    while(((c = getchar()) != '\n') && (c != EOF));
}

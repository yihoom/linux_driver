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
 * ./chrdevbaseApp <filename> <1:2> 1表示读，2表示写
*/
int main(int argc, char *argv[])
{
    int fd = 0;
    int ret = 0;
    char *filename;
    char *writeparam;
    char Readbuf[100];
    char writebuf[100];
    static char userdara[] = {"user data"};

    int OperaFalg = 0;

    if(argc != 3)
    {
        printf("error cmd\r\n");
        return -1;
    }

    filename = argv[1];
    OperaFalg = atoi(argv[2]);


    //open返回一个文件描述符
    //文件操作符详见man 2 open
    fd = open(filename, O_RDWR);

    //open如果出错，则会返回-1
    if(fd < 0)
    {
        printf("Can't open file %s\r\n", filename);
        return -1;
    }

    //read函数
    //如果成功的话，read会返回读到的字节数
    //ssize_t read(int fd, void *buf, size_t count);

    if(OperaFalg == 1)
    {

        ret = read(fd, Readbuf, 50);
    
        if(ret == 0)
        {
            printf("read from driver %s\r\n", Readbuf);
        }

        //read函数如果失败，会返回-1
        if(ret < 0)
        {
            printf("read file %s failed\r\n", filename);
        }
    
    }
    

    /* wrirte函数 */
    //ssize_t write(int fd, const void *buf, size_t count);
    //如果成功write会返回写入的字节数
    if(OperaFalg == 2)
    {   
        memcpy(writebuf, userdara, sizeof(userdara));
        ret = write(fd, writebuf, 20);

        //如果错误，会返回-1
        if(ret < 0)
        {
            printf("write to file %s failed\r\n", filename);
        }
        
    }
    

    /* close */
    //  int close(int fd);
    // close成功的话返回0，失败的话返回-1
    ret = close(fd);
    if(ret < 0)
    {
        printf("close file %s failed\r\n", filename);
    }


    return 0;
}

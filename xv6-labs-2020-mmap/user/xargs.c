#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char* argv[])
{
    //参数个数少于2打印错误提示
    if(argc < 2)
    {
        fprintf(2, "Usage: xargs command\n");
        exit(1);
    }
    //存放连接后的命令参数
    char* argv2[MAXARG];
    //用于数组位置定位
    int pos1 = 0;
    int pos2 = 0;
    //记录读出内容的大小
    int n;
    //用于连接从缓冲区读出的内容
    char temp[512];
    //用于标记缓冲区的位置
    char* p = temp;
    //读取xrags命令后的参数
    for(int i = 1; i < argc; i++)
    {
        argv2[pos1++] = argv[i];
    }
    //存放从缓冲区读出的内容
    char buf[512];
    //从缓冲区中读取左边指令的输出结果
    while ((n = read(0, buf, sizeof(buf))) > 0)
    {
        for(int j = 0;j < n;j++)
        {
            //读到换行符
            if(buf[j] == '\n')
            {
                //父进程进行参数连接
                temp[pos2] = 0;
                pos2 = 0;
                argv2[pos1++] = p;
                p = temp;
                argv2[pos1] = 0;
                pos1 = argc - 1;
                //子进程执行指令
                if(fork() == 0)
                {
                    exec(argv[1], argv2);
                }
                wait(0);
            }
            //读到空格
            else if(buf[j] == ' ')
            {
                temp[pos2++] = 0;
                argv2[pos1++] = p;
                p = &temp[pos2];
            }
            else
            {
                temp[pos2++] = buf[j];
            }
        }
    }
    exit(0);
}
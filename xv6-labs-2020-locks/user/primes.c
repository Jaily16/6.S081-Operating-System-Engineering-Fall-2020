//使用管道和进程输出质数
#include "kernel/types.h"
#include "user/user.h"

//宏定义读和写的标志
#define FLAG_READ 0
#define FLAG_WRITE 1

//进程接收输入和传递到下一个进程函数
void filterAndPass(int previous[2])
{
  //关闭上一个管道的写端
  close(previous[FLAG_WRITE]);
  int outNum;
  //从上一个进程传递过来的管道已经为空，结束进程
  if(read(previous[FLAG_READ],&outNum,sizeof(outNum)) == 0)
  {
    close(previous[FLAG_READ]);
    exit(0);
  }
  printf("prime %d\n", outNum);
  //传递到下一个进程的管道
  int next[2];
  pipe(next);
  //用于暂存传递过来的管道中读取的剩下的数字
  int tempNum;
  if(fork() == 0)
  {
    //子进程进行函数的递归
    filterAndPass(next);
  }
  else
  {
    //本进程读取剩余的数并将符合条件的数向后传递
    close(next[FLAG_READ]);
    while(read(previous[FLAG_READ],&tempNum,sizeof(tempNum)) > 0)
    {
      //将本次进程无法整除的数继续向后传递
      if(tempNum % outNum != 0)
      {
        write(next[FLAG_WRITE],&tempNum,sizeof(tempNum));
      }
    }
    close(next[FLAG_WRITE]);
    wait(0);
  }
  
  exit(0);
}

//传递初始值和建立初始管道
void startPrimes(int p[2])
{
  //子进程进行传递函数的调用
  if(fork() == 0)
  {
    filterAndPass(p);
  }
  else
  {
    close(p[FLAG_READ]);
    for(int i = 2;i <= 35;i++)
    {
      write(p[FLAG_WRITE],&i,sizeof(i));
    }
    
    close(p[FLAG_WRITE]);
    
    wait(0);
  }

}

int main()
{
  int fp[2];
  pipe(fp);
  startPrimes(fp);
  exit(0);
}

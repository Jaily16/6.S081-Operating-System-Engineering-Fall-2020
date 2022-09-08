// 管道乒乓球
#include "kernel/types.h"
#include "user/user.h"

int 
main()
{
  int p_parentToChild[2]; //父进程写子进程读管道
  int p_childToParent[2]; //子进程写父进程读管道
  char ch[] = {'L'}; //传递的字节
  int len = sizeof(ch); //字节的长度
  
  // 建立管道
  pipe(p_parentToChild);
  pipe(p_childToParent);
  
  if(fork() == 0)
  {
    close(p_parentToChild[1]);
    close(p_childToParent[0]);
    
    //子进程读
    if(read(p_parentToChild[0],ch,len) != len){
      fprintf(2,"child process read error!\n");
      exit(1);
    }
    
    printf("%d: received ping\n",getpid());
    
    //子进程写
    if(write(p_childToParent[1],ch,len) != len){
      fprintf(2,"child process write error!\n");
      exit(1);
    }
    
    exit(0);
  }
  
  close(p_parentToChild[0]);
  close(p_childToParent[1]);

  //父进程写
  if(write(p_parentToChild[1],ch,len) != len){
    fprintf(2,"parent process write error!\n");
    exit(1);
  }
    
  //父进程读
  if(read(p_childToParent[0],ch,len) != len){
    fprintf(2,"parent process read error!\n");
    exit(1);
  }

  
  printf("%d: received pong\n",getpid());
 
  wait(0);
  exit(0);
}

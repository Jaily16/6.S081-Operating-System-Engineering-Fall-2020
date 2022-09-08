// 将控制台输入输入输出到控制台

#include "kernel/types.h"
#include "user/user.h"

int
main()
{
  char buf[64];
  
  while(1){
    int n = read(0,buf,sizeof(buf));
    if(n<=0)
      break;
    write(1,buf,n);
    break;
  }
  
  exit(0);
}

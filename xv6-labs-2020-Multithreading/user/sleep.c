//使系统定时睡眠
#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[]){
  if(argc < 2){
    fprintf(2,"Usage: sleep time...\n");
    exit(1);
  }
  
  int sleepTime = atoi(argv[1]);
  printf("(nothing happens for a little while)\n");
  sleep(sleepTime);
  exit(0);
}

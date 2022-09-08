#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

//将路径转换为最后的后缀文件名
char* fmtname(char *path)
{
  char *p;

  //定位到后缀文件名
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  //返回文件名
  return p;
}

//比较并打印相同文件名的完整文件路径
void compareAndPrintPath(char* filePath, char* findName)
{
  if(strcmp(fmtname(filePath),findName) == 0)
  {
    printf("%s\n", filePath);
  }
}

//在给定的路径中查找文件
void find(char* path, char* findName)
{
  char buf[512], *p;  //用于临时存放路径和定位
  int fd;  //打开的文件描述符
  struct dirent de;  //存放目录下的文件信息
  struct stat st;  //存放文件的类型信息
  
  if((fd = open(path, O_RDONLY)) < 0)
  {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0)
  {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }
  
  switch(st.type)
  {
  case T_FILE:
    compareAndPrintPath(path, findName);
    break;

  case T_DIR:
    //检查路径是否过长
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    //将路径拷到buf中
    strcpy(buf, path);
    //将p定位到路径最后
    p = buf+strlen(buf);
    //加上分隔符
    *p++ = '/';
    //读取文件夹中的文件
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name,"..") == 0)
        continue;
      //将子目录复制到buf中
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      find(buf, findName);
    }
    break;
  }
  close(fd);
} 

int main(int argc, char* argv[])
{
  if(argc != 3)
  {
    fprintf(2,"Usage: find <dirName> <fileName>\n");
    exit(1);
  }
  
  find(argv[1],argv[2]);
  exit(0);
}

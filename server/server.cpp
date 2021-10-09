#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <fcntl.h>
#include <

// 用于获取文件信息
#include <sys/stat.h>
// 在两个文件描述符之间出传输数据（完全内核操作）
#include <sys/sendfile.h>

int main(int argc, char* argv[]) {

  int listenfd = 0, connfd = 0;

  // 定义服务器以及客户端的socket地址
  struct sockaddr_in serv_addr, client;

  int nAddrLen = sizeof(client);  // 保存客户端接收的

  // 创建socket并获得socket文件描述字
  listenfd = socket(AF_INET, SOCK_STREAM, 0);

  // 发送接收的buf
  void* sendbuf = NULL;
  void* recvbuf = NULL;

  // 给发送和接收的buf分配内存空间
  sendbuf = malloc(sizeof(char) * 1024);
  recvbuf = malloc(sizeof(char) * 1024);

  // 初始化server地址以及发送buf
  memset(&serv_addr, '0', sizeof(serv_addr));
  memset(sendbuf, '0', 1024);

  serv_addr.sin_family = AF_INET;
  // 监听本机的所有网卡ip
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  // 绑定输入的端口号
  serv_addr.sin_port = htons(atoi(argv[1]));

  // 将描述字绑定名字
  bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
  
  // 持续监听本地地址，且server的最大连接个数为10
  listen(listenfd, 10);
  
  // 接受一个新连接,不指定特定的连接方式
  connfd = accept(listenfd, NULL, NULL);

  char buf[100], command[5], filename[20], extension[20], lscommand[20];

  struct stat obj;
  int recv_size;
  int count;
  FILE* file_handle;  // 文件指针
  int already_exists = 0;
  int overwrite_choice = 0;
  char* pos = NULL;

  // 循环
  while (1) {
    // 从tcp的另一端接收数据，将接收缓冲区中的数据存储在buf中
    recv(connfd, buf, 100, 0);

    // 从buf中读取格式化输入，即以空格区分的第一个字符串
    sscanf(buf, "%s", command);

    /***********************************************************************                    
      当前客户端发送的命令为put         
     ***********************************************************************/
    if (!strcmp(command, "put")) {
      char* recv_buf;
      // 读取buf中的下一个字符串
      sscanf(buf + strlen(command), "%s", filename);
      
      // 判断put的文件在ftp server中是否存在,并向client发送应答
      if (access(filename, F_OK) != -1) {
        already_exists = 1;
        // send发送时需要比较发送数据的长度和connfd的缓冲区size，
        // 注意此时send仅仅将缓冲区中的数据拷贝到套接字发送缓冲区中的缓存中，但是数据并没有发送
        send(connfd, &already_exists, sizeof(int), 0);
      }
      else {
        already_exists = 0;
        send(connfd, &already_exists, sizeof(int), 0);
      }
      // 客户端根据server收到的反馈信号，发送对应的overwrite信号，server接收
      recv(connfd, &overwrite_choice, sizeof(int), 0);

      // judge the overwrite signal
      if (already_exists == 1 && overwrite_choice == 1) {
        // 文件在server中存在，且客户端发送覆盖信号
        file_handle = fopen(filename, "w");

        recv(connfd, &recv_size, sizeof(int), 0);  // 首先接收需要发送的数据的大小

        recv_buf = (char*)malloc(recv_size);  // 根据接收到的file文件size分配接收缓冲的大小
        recv(connfd, recv_buf, recv_size, 0);

        count = (int)fwrite(recv_buf, recv_size, sizeof(char), file_handle);
        printf("file size is : %d\n", count);

        fclose(file_handle);
        // 发送当前状态
        send(connfd, &count, sizeof(int), 0);

      }
      else if (already_exists == 0 && overwrite_choice == 1) {
        // 当前文件不存在，需要先创建
        file_handle = fopen(filename, "w");
        recv(connfd, &recv_size, sizeof(recv_size), 0);
        recv_buf = (char*)malloc(recv_size);
        count = fwrite(recv_buf, recv_size, sizeof(char), file_handle);
        printf("file size is : %d\n", count);
        fclose(file_handle);
        send(connfd, &count, sizeof(int), 0);
      }
    }  // if (!strcmp(command, "put"))
    /***********************************************************************                    
      当前客户端发送的命令为get       
     ***********************************************************************/
    else if (!strcmp(command, "get")) {
      sscanf(buf, )


      // 根据文件名获取文件信息并保存于obj中
      stat(filename, &obj);

    }  // else if (!strcmp(command, "get"))
    /***********************************************************************                    
      当前客户端发送的命令为mget 
     ***********************************************************************/
    else if (!strcmp(command, "mget")) {

    }  // else if (!strcmp(command, "mget"))
    /***********************************************************************                    
      当前客户端发送的命令为quit       
     ***********************************************************************/
    else if (!strcmp(command, "quit")) {

    }  // else if (!strcmp(command, "quit"))

  }  // while (1)

  return 0;
}
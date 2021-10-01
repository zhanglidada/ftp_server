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
  FILE* file_handle;  // 文件指针
  int already_exists = 0;
  int overwrite_choice = 0;
  char* pos = NULL;

  // 循环
  while (1) {
    // 从tcp的另一端接收数据，接收的命令字符存储于buf中
    recv(connfd, buf, 100, 0);

    // 从buf中读取格式化输入
    sscanf(buf, "%s", command);

    /***********************************************************************                    
      当前客户端发送的命令为put         
     ***********************************************************************/
    if (!strcmp(command, "put")) {
      char* recv_buf;
      sscanf(buf + strlen(command), "%s", filename);
      stat(filename, &obj);
      
      // 判断put的文件在ftp server中是否存在
      if (access(filename, F_OK) != -1) {
        already_exists = 1;
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

        recv_buf = (char*)malloc(recv_size);  // 根据
        recv(connfd, recv_buf, recv_size, 0);

      }
      else if (already_exists == 0 && overwrite_choice == 1) {

      }
    }  // if (!strcmp(command, "put"))
    else if (!strcmp(command, "get")) {

    }  // else if (!strcmp(command, "get"))
    else if (!strcmp(command, "mget")) {

    }  // else if (!strcmp(command, "mget"))
    else if (!strcmp(command, "quit")) {

    }  // else if (!strcmp(command, "quit"))

  }  // while (1)

  return 0;
}
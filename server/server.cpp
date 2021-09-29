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

  // 循环
  while (1)
  {
    // 从tcp的另一端接收数据
    recv(connfd, buf, 100, 0);

    sscanf(buf, "%s", command);
  }




  return 0;
}
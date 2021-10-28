// gcc client.c -o client
// ./client <ip_add> <port>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <sys/stat.h>

// 根据server的ip以及port进行连接
int main(int argc,char *argv[]) {
  int sockfd = 0;  // socket文件描述符
  char recvbuf[1024];  // 接收消息的缓冲区
  struct sockaddr_in serv_addr;

  // 启动客户端时输入了非法的参数
  if (argc != 3) {
    printf("\n Usage: %s <ip of server> \n",argv[0]);		// checking argument 
    return 1;
  }

  memset(recvbuf, '0', sizeof(recvbuf));

  // 创建socket套接字
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Error : Could not create socket. \n");
    return 1;
  }

  // assign server address
  memset(&serv_addr, '0', sizeof(serv_addr));

  // 设定服务器的信息
  serv_addr.sin_family = AF_INET;
  // 将点分文本的ip地址转换成二进制网络字节序的ip地址
  if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0)
  {
    printf("\n inet_pton error occured\n");
    return 1;
  }
  serv_addr.sin_port = htons(atoi(argv[2]));  // 从主机字节序转换为网络字节序

  if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\n Error: Connect Fialed \n");
    return 1;
  }

  












      
  return 0;
}
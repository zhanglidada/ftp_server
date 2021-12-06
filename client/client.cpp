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
#include <sys/sendfile.h>
#include <fcntl.h>

/*
  根据server的ip以及port进行连接
 */
int main(int argc,char *argv[]) {
  int       sockfd = 0;   // socket文件描述符
  char      recvbuf[1024];  // 接收消息的缓冲区
  struct    sockaddr_in serv_addr;

  int       choice = 0;  // 用于发送客户端做出的选择
  char      choice_str[1024];  // 客户端的选择
  int       choice_str_len = 0;
  char      filename[20];  // 文件名
  char      buf[100];  // 发送缓冲
  char*     recv_buf = NULL;  // 接受缓存
  char      ext[20];
  int       filehandle = 0;  // 文件描述符
  int       already_exists = 0;  // 判断需要发送的文件是否在服务端存在
  int       overwrite_choice = 1;  // 文件覆盖选项
  struct    stat obj;  //  保存文件的状态信息
  int       filesize, status = 0;

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
  serv_addr.sin_port = htons(atoi(argv[2]));  // 将端口号从主机字节序转换为网络字节序

  // 客户端连接server
  if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\n Error: Connect Fialed \n");
    return 1;
  }
  
  // 客户端接受命令并发送
  while (1) {
    printf("Enter a choice:\n 1- put\n 2- get\n 3- mput\n 4-mget\n 5-quit\n");
    fgets(choice_str, 1024, stdin);
    if ((choice_str_len = strlen(choice_str)) > 1) {
      printf("error input selection!");
      continue;
    }

    choice_str[choice_str_len - 1] = '\0';  // 可以输入任意字符，但是只有满足条件的字符才有效
    choice = atoi(choice_str);  // 从string数据类型转换为int整型

    // 根据输入的选项进行判断
    switch (choice) {
      memset(buf, sizeof(buf), '\0');  // reset buf every time
      // 客户端将文件推送到服务端
      case 1:
        printf("Enter the filename to put in server\n");
        scanf("%s", filename);

        // 文件读取失败，重新读取
        if (access(filename, F_OK) == -1) {
          printf("%s does not exists in client side\n", filename);
          break;
        }

        filehandle = open(filename, O_RDONLY);

        // 发送缓存并接受文件状态
        strcpy(buf, "put ");
        strcat(buf, filename);
        send(sockfd, buf, sizeof(buf), 0);  // 发送client输入的命令
        recv(sockfd, &already_exists, sizeof(int), 0);  // 从服务端接收要发送的文件在服务端的状态

        // 客户端发送的文件在服务端已存在
        if (already_exists) {
          printf("same name file already exits in server 1. overwirte 2.NO overwirte\n");
          scanf("%d", &overwrite_choice);
        }
        send(sockfd, &overwrite_choice, sizeof(int), 0);  // 如果客户端要发送的文件不在服务器内，默认方式发送文件

        // 只有当客户端发送的文件 在服务端不存在/在服务端已存在，但是强制覆盖
        if (overwrite_choice == 1) {
          stat(filename, &obj);  // 获取文件状态
          filesize = obj.st_size;
          send(sockfd, &filesize, sizeof(int), 0);  // 发送文件size
          sendfile(sockfd, filehandle, NULL, filesize);  // 减少io消耗
          recv(sockfd, &status, sizeof(int), 0);

          if (status)
            printf("%s File stored successfully\n",filename);
          else
            printf("%s File failed to be stored to remote machine\n" , filename); 
        }
        break;

      // get file from server
      case 2:
        printf("Enter filename to get:");
        scanf("%s", filename);
        strcpy(buf, "get ");
        strcat(buf, filename);

        // send file cmd to server
        send(sockfd, buf, 100, 0);

        recv(sockfd, &filesize, sizeof(int), 0);  // recv filesize of the file to get
        if (!filesize){
          printf("No such file in remote directory");
          break;
        }

        // 对本地存在的文件覆盖选项,重置overwrite值
        if (access(filename, F_OK) != -1) {
          already_exists = 1;
          printf("same name file already exits in client 1. overwirte 2.NO overwirte\n");		// file already exits
          scanf("%d", overwrite_choice);
        }

        if (overwrite_choice && already_exists) {
          // 覆盖存在的文件
          filehandle = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 644);
          
          // 分配对应大小的内存
          recv_buf = (char*)malloc(filesize);
          recv(sockfd, recv_buf, filesize, 0);
          write(filehandle, recv_buf, filesize);
          close(filehandle);

        } else if (overwrite_choice && !already_exists) {
          // 打开一个新文件
          filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 666);
          recv(sockfd, recv_buf, filesize, 0);
          write(filehandle, recv_buf, filesize);
          close(filehandle);
        }
        break;

      // mput file to server
      case 3:
      
      // mget file from server
      case 4:

      // quit
      case 5:
      strcpy(buf, "quit");
      send(sockfd, buf, 100, 0);
      recv(sockfd, &status, sizeof(int), 0);
      
      // 退出服务器
      if (status) {
        printf("server closed\n Quitting...\n");
        exit(0);
      }
      printf("Server failed to close connection\n");		// faild to quit
      break;
    }
  }











      
  return 0;
}
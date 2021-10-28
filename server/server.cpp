// gcc server.c -o server
// ./server <port>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
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

  struct stat obj;
  int recv_size             = 0;
  int file_size             = 0;
  int count                 = 0;
  int file_handle           = 0;  // 文件描述符
  int already_exists        = 0;
  int overwrite_choice      = 0;
  int status                = 0;
  char* pos                 = NULL;
  int num_lines             = 0;  // 传输文件的数量
  int ch                    = 0;  // 文件读取的返回值

  // 循环
  while (1) {
    // 从tcp的另一端接收数据，将接收缓冲区中的数据存储在buf中
    recv(connfd, buf, 100, 0);

    // 从buf中读取格式化输入，即以空格区分的第一个字符串，区分客户端发送的命令模式
    sscanf(buf, "%s", command);

    /***********************************************************************                    
      当前客户端发送的命令为put, 将客户端本地文件推送到服务器
      如果文件在server中存在，server要向client发送是否覆盖的消息
     ***********************************************************************/
    if (!strcmp(command, "put")) {
      char* recv_buf;
      // 读取buf中的下一个字符串
      sscanf(buf + strlen(command), "%s %s", filename, filename);
      
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
        file_handle = open(filename, O_RDWR);  // 读写的形式打开

        recv(connfd, &recv_size, sizeof(int), 0);  // 首先接收需要发送的数据的大小

        recv_buf = (char*)malloc(recv_size);  // 根据接收到的file文件size分配接收缓冲的大小
        recv(connfd, recv_buf, recv_size, 0); 

        count = (int)write(file_handle, recv_buf, recv_size);
        printf("file size is : %d\n", count);

        close(file_handle);
        // 发送当前状态
        send(connfd, &count, sizeof(int), 0);

      }
      else if (already_exists == 0 && overwrite_choice == 1) {
        // 当前文件不存在，需要先创建
        file_handle = open(filename, O_RDWR);
        recv(connfd, &recv_size, sizeof(recv_size), 0);
        recv_buf = (char*)malloc(recv_size);
        count = (int)write(file_handle, recv_buf, recv_size);
        printf("file size is : %d\n", count);
        close(file_handle);
        send(connfd, &count, sizeof(int), 0);
      }
    }  // if (!strcmp(command, "put"))
    /***********************************************************************                    
      当前客户端发送的命令为get，客户端发送get命令，从服务端下载文件
     ***********************************************************************/
    else if (!strcmp(command, "get")) {
      // 客户端发送的消息，去除命令的偏移后，第一个字符串即为服务器上的文件
      sscanf(buf + strlen(command), "%s", filename);

      // 根据文件名获取文件信息并保存于obj中
      stat(filename, &obj);

      // 此时客户端从服务器下载文件，只需要以读模式打开文件
      file_handle = open(filename, O_RDONLY);

      file_size = obj.st_size;  // 获取文件的size，返回给客户端

      if (file_handle == NULL)
        file_size = 0;

      // 发送文件size给客户端
      send(connfd, &file_size, sizeof(int), 0);

      // get获取的文件不在服务器上，继续循环，等待客户端的下一次发送
      if (file_size == 0)
        continue;
      
      // 接收客户端发送的overwrite信号
      recv(connfd, &overwrite_choice, sizeof(int), 0);

      // 零拷贝发送数据
      if (overwrite_choice == 1)
        sendfile(connfd, file_handle, NULL, file_size);
    }  // else if (!strcmp(command, "get"))
    /***********************************************************************                    
      当前客户端发送的命令为mget,下载服务器上的多个文件，支持通配符
      此处使用了系统的ls命令，且客户端发送的是需要获取的文件类型
     ***********************************************************************/
    else if (!strcmp(command, "mget")) {
      char* line                = NULL;  // 读取的文件行内容
      size_t len                = 0;
      ssize_t read              = 0;  // signed size_t

      sscanf(buf + sizeof(command), extension);
      strcpy(lscommand, "ls *.");
      strcat(lscommand, extension);
      strcat(lscommand, "> filelist.txt");
      // 执行linux系统命令
      system(lscommand);

      // 打开=需要下载的文件列表
      FILE* fp = fopen("filelist.txt", "r");
      
      // 计算得到文件列表的数目
      while (!feof(fp)) {
        // 获取文件指针流中的一个字符，做为无符号数读取并转换成整数
        ch = fgetc(fp);
        if (ch == '\n')
          num_lines ++;
      }

      // 重新定位文件指针到文件头
      fseek(fp, 0, SEEK_SET);

      send(connfd, &num_lines, sizeof(int), 0);  // 发送server接收到的需要mget的文件数量

      // 发送所有文件到客户端
      while (read = getline(&line, &len, fp) != -1) {
        // getline获取的行数据以 '\n' 结尾
        if ((pos = strchr(line, '\n')) != NULL)
          *pos = '\0';

        strcpy(filename, line);  // 将文件名进行拷贝
        
        send(connfd, filename, 20, 0);  // 发送文件名
        
        stat(line, &obj);
        // 以只读模式打开文件
        file_handle = open(filename, O_RDONLY);
        file_size = obj.st_size;
        send(connfd, &file_size, sizeof(int), 0);  // 发送文件size
        // server 接收客户端发送的文件接受命令
        recv(connfd, &overwrite_choice, sizeof(int), 0);

        if (file_size && overwrite_choice == 1) {
          // 零拷贝发送文件
          sendfile(connfd, file_handle, NULL, file_size);
        }
      }

      // 关闭并删除文件
      fclose(fp);
      remove("filelist.txt");

    }  // else if (!strcmp(command, "mget"))
    /***********************************************************************                    
      当前客户端发送的命令为quit，退出ftp服务   
     ***********************************************************************/
    else if (!strcmp(command, "quit")) {
      printf("FTP server quitting..\n");
      status = 1;
      // 发送结束状态
      send(connfd, &status, sizeof(int), 0);
      exit(0);  // server退出
    }  // else if (!strcmp(command, "quit"))

  }  // while (1)

  return 0;
}
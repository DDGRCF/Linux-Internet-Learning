#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/wait.h>

#include "net_utils.h"

void signal_handler(int num) {
  printf("quit:%d...\n", num);
}

void catch_child(int num) {
  pid_t wpid;
  int status;
  if ((wpid = waitpid(0, &status, WNOHANG)) != -1) {
    if (WIFEXITED(status)) {
      printf("catch child: %d, ret=%d\n", wpid, WEXITSTATUS(status));
    } else if (WIFSTOPPED(status)) {
      printf("catch child: %d, ret=%d\n", wpid, WSTOPSIG(status));
    }
  }
}

int main() {
  signal(SIGINT, signal_handler);
  struct sigaction act;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  act.sa_handler = catch_child;
  sigaction(SIGCHLD, &act, nullptr);

  const int Port = 12556;
  int ret;

  struct sockaddr_in serv_addr, clit_addr;
  socklen_t clit_len;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(Port);

  char sip[INET_ADDRSTRLEN]{0};
  if ((inet_ntop(AF_INET, &serv_addr.sin_addr, sip, sizeof(sip))) == nullptr) {
    perr_exit("inet_pton", errno);
  }
  printf("begin server: %s:%d ...\n", sip, Port);

  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  my_bind(sfd, reinterpret_cast<struct sockaddr*>(&serv_addr), sizeof(serv_addr));
  my_listen(sfd, 100);

  while (true) {
    int cfd;
    char cip[INET_ADDRSTRLEN]{0};
    memset(&clit_addr, 0, sizeof(clit_addr));
    if ((cfd = my_accept(sfd, reinterpret_cast<struct sockaddr*>(&clit_addr), &clit_len)) == -1) {
      perr_exit("my_accept", cfd);
    }
    if (inet_ntop(AF_INET, &clit_addr.sin_addr, cip, sizeof(cip)) == nullptr) {
      perr_exit("inet_ntop", errno);
    }
    int cport = ntohs(clit_addr.sin_port);  
    printf("--accept client-%s:%d\n", cip, cport);
    if (fork() == 0) {
      printf("--create new process: %d...\n", getpid());
      int n;
      char buf[BUFSIZ];
      for (;;) {
        if ((n = my_read(cfd, buf, sizeof(buf))) == -1) { // 读取是否出错
          perr_exit("my_read", n);
        }
        if (n == 0) { // 如果为0，说明达到了文件夹末端，也就是有一端关闭了
          printf("other side has closed\n");
          break;
        }
        for (int i = 0; i < n; i++) {
          buf[i] = toupper(buf[i]);
        }

        if ((n = my_write(cfd, buf, sizeof(buf))) == -1) { // 写入是否出错
          perr_exit("my_write", n);
        }
      }
      close(cfd);
      printf("--exit process: %d\n", getpid());
      exit(0);
    }
  }
  return 0;
}
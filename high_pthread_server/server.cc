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
#include <pthread.h>

#include "net_utils.h"

const int Port = 12556;
const int kMaxThread = 100;


struct ThreadInfo {
  int fd;
  struct sockaddr_in addr;
};

void* deal_func(void* arg) {
  struct ThreadInfo* ti = static_cast<ThreadInfo*>(arg);
  printf("--create new thread: %d...\n", getpid());
  int n;
  char buf[BUFSIZ];
  for (;;) {
    if ((n = my_read(ti->fd, buf, sizeof(buf))) == -1) { // 读取是否出错
      perr_exit("my_read", n);
    }
    if (n == 0) { // 如果为0，说明达到了文件夹末端，也就是有一端关闭了
      printf("other side has closed\n");
      break;
    }
    for (int i = 0; i < n; i++) {
      buf[i] = toupper(buf[i]);
    }

    if ((n = my_write(ti->fd, buf, sizeof(buf))) == -1) { // 写入是否出错
      perr_exit("my_write", n);
    }
  }
  close(ti->fd);
  printf("--exit thread: %d\n", getpid());

  return static_cast<void*>(0);
}

int main() {
  int ret;

  struct ThreadInfo t_info[kMaxThread];

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

  int i = 0;
  while (true) {
    int cfd;
    char cip[INET_ADDRSTRLEN]{0};
    memset(&clit_addr, 0, sizeof(clit_addr));
    if ((cfd = my_accept(sfd, reinterpret_cast<struct sockaddr*>(&clit_addr), &clit_len)) == -1) {
      perr_exit("my_accept", cfd);
    }
    if (i > kMaxThread - 1) {
      printf("reach max num...\n");
      close(cfd);
      continue;
    }
    if (inet_ntop(AF_INET, &clit_addr.sin_addr, cip, sizeof(cip)) == nullptr) {
      perr_exit("inet_ntop", errno);
    }
    int cport = ntohs(clit_addr.sin_port);  
    printf("--accept client-%s:%d\n", cip, cport);

    t_info[i].fd = cfd; 
    t_info[i].addr = clit_addr;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // pthread_detach
    pthread_t pid;
    ret = pthread_create(&pid, &attr, deal_func, &t_info[i++]);
    if (ret == -1) {
      perr_exit("pthread_create", ret);
    }
  }
  return 0;
}
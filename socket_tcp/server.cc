#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>


void say_error(const char* msg, int num) {
  fprintf(stderr, "%s, errno=%d:%s", msg, num, strerror(num));
  exit(1);
}

int main() {
  int ret;
  const int Port = 12555;
  const int kMaxListen = 100;

  struct sockaddr_in serv_addr, clit_addr;
  socklen_t clit_addr_len;
  char buf[BUFSIZ];
  char clit_ip[INET_ADDRSTRLEN];

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(Port);

  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    say_error("server socket", sfd);
  }
  ret = bind(sfd, reinterpret_cast<struct sockaddr *>(&serv_addr), sizeof(serv_addr)); 
  if (ret == -1) {
    say_error("bind socket", ret);
  }
  ret = listen(sfd, kMaxListen);
  if (ret == -1) {
    say_error("listen socket", ret);
  }
  
  for (;;) {
    clit_addr_len = sizeof(clit_addr);
    int cfd = accept(sfd, reinterpret_cast<struct sockaddr *>(&clit_addr), &clit_addr_len);
    if (cfd == -1) {
      say_error("client socket", ret);
    }
    memset(buf, 0, sizeof(buf));
    ret = read(cfd, buf, sizeof(buf));
    printf("received from %s at Port: %d\n", buf, Port); 
    memset(clit_ip, 0, sizeof(clit_ip));
    if (inet_ntop(AF_INET, reinterpret_cast<void *>(&clit_addr), 
        clit_ip, sizeof(clit_ip)) == nullptr) {
      say_error("client inet_ntop", errno);
    }
    int clit_port = ntohs(clit_addr.sin_port);
    printf("send msg to %s:%d\n", clit_ip, clit_port);
    for (int i = 0; i < ret; i++) {
      buf[i] = toupper(buf[i]);
    }
    write(cfd, buf, ret);
    close(cfd);
  }
  return 0;
}
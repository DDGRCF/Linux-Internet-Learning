#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>


void say_error(const char* msg, int num) {
  fprintf(stderr, "%s, errno=%d:%s", msg, num, strerror(num));
  exit(1);
}

int main() {
  int ret;
  struct sockaddr_in clit_addr; 
  const char* IP = "127.0.0.1";
  const int Port = 12555;
  memset(&clit_addr, 0, sizeof(clit_addr));
  int cfd = socket(AF_INET, SOCK_STREAM, 0);
  clit_addr.sin_family = AF_INET;
  clit_addr.sin_port = htons(Port);
  ret = inet_pton(AF_INET, IP, &clit_addr.sin_addr);
  if (ret != 1) {
    say_error("inet_pton", ret);
  }
  ret = connect(cfd, reinterpret_cast<sockaddr *>(&clit_addr), sizeof(clit_addr));
  if (ret == -1) {
    say_error("connect", ret);
  }
  char* buf;
  char recv_buf[BUFSIZ];
  size_t len;
  ret = getline(&buf, &len, stdin); // getline读入\n
  ret = write(cfd, buf, len);
  printf("write %s:%d\n", buf, ret);
  ret = read(cfd, recv_buf, sizeof(recv_buf));
  if (ret > 0) {
    printf("read %s:%d\n", recv_buf, ret);
  }
  free(buf);
  close(cfd);
  return 0;
}

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void say_error(const char* msg, int num) {
  fprintf(stderr, "%s, errno=%d:%s", msg, num, strerror(num));
  exit(1);
}

int main() {
  int ret;
  struct sockaddr_in clit_addr;
  const char* IP = "127.0.0.1";
  const int Port = 12556;
  memset(&clit_addr, 0, sizeof(clit_addr));
  int cfd = socket(AF_INET, SOCK_STREAM, 0);
  clit_addr.sin_family = AF_INET;
  clit_addr.sin_port = htons(Port);
  ret = inet_pton(AF_INET, IP, &clit_addr.sin_addr);
  if (ret != 1) {
    say_error("inet_pton", ret);
  }
  ret =
      connect(cfd, reinterpret_cast<sockaddr*>(&clit_addr), sizeof(clit_addr));
  if (ret == -1) {
    say_error("connect", ret);
  }
  char recv_buf[BUFSIZ];
  size_t len;
  for (;;) {
    char* buf = nullptr;
    memset(recv_buf, 0, sizeof(recv_buf));
    ret = getline(&buf, &len, stdin);  // getline读入\n
    ret = write(cfd, buf, len);
    if (ret < 0) {
      say_error("write", ret);
    }
    if (ret == 0) {
      printf("other side close\n");
      free(buf);  // 在循环中c++free完需要使用nullptr，进行重新赋值
      break;
    }
    printf("write %s:%ld\n", buf, strlen(buf));
    ret = read(cfd, recv_buf, sizeof(recv_buf));
    if (ret < 0) {
      say_error("read", ret);
    }
    if (ret == 0) {
      printf("other side close\n");
      free(buf);  // 在循环中c++free完需要使用nullptr，进行重新赋值
      break;
    }
    printf("read %s:%ld\n", recv_buf, strlen(recv_buf));
    free(buf);  // 在循环中c++free完需要使用nullptr，进行重新赋值
  }

  close(cfd);
  return 0;
}

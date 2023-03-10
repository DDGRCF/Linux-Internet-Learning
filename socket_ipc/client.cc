#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

void perr_exit(const char* s, int num = -1) {
  if (num == -1) {
    num = errno;
  }
  fprintf(stderr, "%s, errno = %d: %s\n", s, num, strerror(num));
  exit(1);
}

int main() {
  int ret, fd;
  struct sockaddr_un addr;
  const char* serv_name = "foo.socket";
  const char* clit_name = "clit.socket";

  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    perr_exit("socket", fd);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, clit_name);
  int len = offsetof(struct sockaddr_un, sun_path) + strlen(clit_name);
  unlink(clit_name);

  ret = bind(fd, reinterpret_cast<sockaddr*>(&addr), len);
  if (ret < 0) {
    perr_exit("bind", ret);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, serv_name);
  len = offsetof(struct sockaddr_un, sun_path) + strlen(serv_name);
  ret = connect(fd, reinterpret_cast<sockaddr*>(&addr), len);
  if (ret < 0) {
    perr_exit("connect", ret);
  }

  char buf[1024];
  while (fgets(buf, sizeof(buf), stdin) != nullptr) {
    ret = write(fd, buf, sizeof(buf));
    if (errno == ECONNRESET || ret == 0) {
      perr_exit("write", ret);
    }
    ret = read(fd, buf, sizeof(buf));
    printf("read: %s", buf);
  }
  return 0;
}
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

void perr_exit(const char* s, int num = -1) {
  if (num == -1) {
    num = errno;
  }
  fprintf(stderr, "%s, errno = %d: %s\n", s, num, strerror(num));
  exit(1);
}

int main() {
  int ret, sfd, cfd;
  struct sockaddr_un addr;
  struct sockaddr_un clit_addr;
  socklen_t clit_addr_len;
  uid_t cuid;

  struct stat statbuf;

  const char* name = "foo.socket";
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, name);
  int len = offsetof(struct sockaddr_un, sun_path) + strlen(name);
  unlink(name);

  sfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (ret < 0) {
    perr_exit("socket", ret);
  }
  ret = bind(sfd, reinterpret_cast<struct sockaddr*>(&addr), len);
  if (ret < 0) {
    perr_exit("bind", ret);
  }
  ret = listen(sfd, 100);
  if (ret < 0) {
    perr_exit("listen", ret);
  }

  while (true) {
    memset(&clit_addr, 0, sizeof(clit_addr));

    clit_addr_len = sizeof(clit_addr);
    cfd = accept(sfd, reinterpret_cast<struct sockaddr*>(&clit_addr),
                 &clit_addr_len);
    if (cfd < 0) {
      perr_exit("accept", cfd);
    }

    clit_addr_len -= offsetof(struct sockaddr_un, sun_path);
    clit_addr.sun_path[clit_addr_len] = '\0';
    printf("client bind filename: %s\n", clit_addr.sun_path);

    ret = stat(clit_addr.sun_path, &statbuf);
    if (ret < 0) {
      perr_exit("stat", ret);
    }

    if (S_ISSOCK(statbuf.st_mode) == 0) {
      perr_exit("not socket", -1);
    }

    cuid = statbuf.st_uid;

    int n;
    char buf[BUFSIZ];
    while (true) {
      n = read(cfd, buf, sizeof(buf));
      if (errno == ECONNRESET || n == 0) {
        perr_exit("other side has closed");
      } else if (errno == EINTR) {
        printf("interrupt by int");
        continue;
      }
      for (int i = 0; i < n; i++) {
        buf[i] = toupper(buf[i]);
      }
      n = write(cfd, buf, n);
      if (errno == ECONNRESET || n == 0) {
        perr_exit("other side has closed");
      }
    }
    close(cfd);
  }

  close(sfd);

  return 0;
}
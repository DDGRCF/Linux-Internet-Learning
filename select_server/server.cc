#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

void perr_exit(const char* s, int num=-1) {
  if (num == -1) {
    num = errno;
  }
  fprintf(stderr, "%s, errno=%d: %s\n", s, num, strerror(num));
  exit(1);
}

const int Port = 12667;
const int kMaxListen = 100;
const int kMaxConnect = kMaxListen;

int main() {
  int ret, index, maxi, nready, maxfd, cfd;
  struct sockaddr_in serv_addr, clit_addr;
  socklen_t clit_len; char clit_ip[INET_ADDRSTRLEN];
  int clit_port; int clits[FD_SETSIZE];

  // 服务器端口初始化
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(Port);

  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    perr_exit("server socket", sfd);
  }

  ret = bind(sfd, reinterpret_cast<struct sockaddr*>(&serv_addr), sizeof(serv_addr));
  if (ret == -1) {
    perr_exit("server bind", ret);
  }

  ret = listen(sfd, kMaxListen);
  if (ret == -1) {
    perr_exit("server listen", ret);
  }

  fd_set rfds, allfds;
  FD_ZERO(&rfds);
  FD_ZERO(&allfds);
  FD_SET(sfd, &allfds);
  maxfd = sfd;
  maxi = maxfd;

  for (int i = 0; i < FD_SETSIZE; i++) {
    clits[i] = -1;
  }

  for (;;) {
    rfds = allfds;
    nready = select(maxfd + 1, &rfds, nullptr, nullptr, nullptr);
    if (ret == -1) {
      perr_exit("server select", ret);
    }

    if (FD_ISSET(sfd, &rfds)) {
      memset(&clit_addr, 0, sizeof(clit_addr));
      memset(clit_ip, 0, sizeof(clit_ip));
      cfd = accept(sfd, reinterpret_cast<sockaddr*>(&clit_addr), &clit_len);
      if (cfd == -1) {
        perr_exit("client accept", cfd);
      }
      if (inet_ntop(AF_INET, &clit_addr.sin_addr, clit_ip, sizeof(clit_ip)) == nullptr) {
        perr_exit("client inet_ntop", errno);
      }
      clit_port = ntohs(clit_addr.sin_port);
      printf("accept client: %s:%d\n", clit_ip, clit_port);

      for (index = 0; index < FD_SETSIZE; index++) {
        if (clits[index] < 0) {
          clits[index] = cfd;  
          break;
        }
      }

      if (index == FD_SETSIZE) {
        perr_exit("server too many clients", EXIT_SUCCESS);
      }

      FD_SET(cfd, &allfds);
      if (cfd > maxfd) {
        maxfd = cfd;
      }
      if (index > maxi) {
        maxi = index;
      }
      
      if (--nready == 0) {
        continue;
      }
    }
    int ufd;
    for (int i = 0; i < maxi; i++) {
      if ((ufd = clits[i]) < 0) {
        continue;
      }

      int n;
      char buf[BUFSIZ];
      if (FD_ISSET(ufd, &rfds)) {
        if ((n = read(ufd, buf, sizeof(buf))) == -1) {
          if (errno == EINTR) { continue; }
          perr_exit("client read", n);
        }

        if (n == 0) {
          close(ufd);
          FD_CLR(ufd, &allfds);
          clits[i] = -1;
          continue;
        }

        for (int i = 0; i < n; i++) {
          buf[i] = toupper(buf[i]);
        }

        if ((n = write(ufd, buf, sizeof(buf))) == -1) {
          if (n == EINTR) { continue; }
          perr_exit("client write", n);
        }

        if (--nready == 0) {
          break;
        }
      }
    }
  }


  return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <sys/socket.h>
#include <poll.h>
#include <ctype.h>

const int Port = 12557;
const int kMaxListen = 100;
const int kOpenMax = kMaxListen;

void perr_exit(const char* s, int num=-1) {
  if (num == -1) {
    num = errno;
  }
  fprintf(stderr, "%s, errno=%d: %s\n", s, num, strerror(num));
  exit(1);
}

int main() {
  int ret, maxi, index, nready;
  int sfd, cfd, maxfd;
  struct sockaddr_in serv_addr, clit_addr;
  socklen_t clit_len;
  struct pollfd clients[kOpenMax];

  char serv_ip[INET_ADDRSTRLEN], clit_ip[INET_ADDRSTRLEN];

  sfd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(Port);

  if (inet_ntop(AF_INET, &serv_addr.sin_addr, serv_ip, sizeof(serv_ip)) == nullptr) {
    perr_exit("inet_ntop", errno);
  } 
  printf("begin serv in %s:%d ...\n", serv_ip, ntohs(serv_addr.sin_port));

  int opt = 1;
  ret = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (ret == -1) {
    perr_exit("setsockopt", ret);
  }
  ret = bind(sfd, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr));
  if (ret == -1) {
    perr_exit("bind", ret);
  }

  ret = listen(sfd, kMaxListen);
  if (ret == -1) {
    perr_exit("listen", ret);
  }

  clients[0].fd = sfd;
  clients[0].events = POLLRDNORM;

  for (int i = 1; i < kOpenMax; i++) {
    clients[i].fd = -1;
  }

  maxi = 0;
  maxfd = sfd;

  for (;;) {
    nready = poll(clients, maxi + 1, -1);
    if (nready < 0) {
      perr_exit("poll",  nready);
    }

    if (clients[0].revents & POLLRDNORM) {
      clit_len = sizeof(clit_addr);
      memset(&clit_addr, 0, clit_len);
      memset(&clit_ip, 0, sizeof(clit_ip));
      cfd = accept(clients[0].fd, reinterpret_cast<sockaddr*>(&clit_addr), &clit_len); 
      if (cfd == -1) {
        perr_exit("accept", ret);
      }

      if (inet_ntop(AF_INET, &clit_addr.sin_addr, clit_ip, sizeof(clit_ip)) == nullptr) {
        perr_exit("inet_ntop", errno);
      }
      printf("accept client: %s:%d\n", clit_ip, ntohs(clit_addr.sin_port));

      for (index = 1; index < kOpenMax; index++) {
        if (clients[index].fd < 0) {
          clients[index].fd = cfd;
          clients[index].events = POLLRDNORM;
          break;
        }
      }

      if (index == kOpenMax) {
        perr_exit("open max", -1);
      }
    
      if (index > maxi) {
        maxi = index;
      }
      if (--nready <= 0) {
        continue; 
      }
    }

    int ufd;
    for (int i = 1; i < maxi + 1; i++) {
      if ((ufd = clients[i].fd) < 0) {
        continue;
      }
      int n; char buf[BUFSIZ];
      if (clients[i].revents & (POLLRDNORM | POLLERR)) {
        if ((n = read(ufd, buf, sizeof(buf))) == -1) {
          if (errno == EINTR) { continue; }
          else if (errno == ECONNRESET) {
            printf("client[%d] aborted connection\n", i);
            close(ufd);
            clients[i].fd = -1;
          } else {
            perr_exit("read error", n);
          }
        } else if (n == 0) {
          close(ufd);
          printf("client[%d] closed connection\n", i);
          clients[i].fd = -1;
          continue;
        } else {
          for (int i = 0; i < n; i++) {
            buf[i] = toupper(buf[i]);
          }

          if ((n = write(ufd, buf, sizeof(buf))) == -1) {
            if (errno == EINTR) { continue; }
            perr_exit("client write", n);
          }
        }

        if (--nready <= 0) {
          break;
        }
      }
    }
  }

  return 0;
}
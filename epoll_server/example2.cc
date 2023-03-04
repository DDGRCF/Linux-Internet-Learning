#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>


void perr_exit(const char* s, int num=-1) {
  if (num == -1) {
    num = errno;
  }
  fprintf(stderr, "%s, errno = %d: %s\n", s, num, strerror(num));
  exit(1);
}

const int kMaxLen = 10;
const int Port = 12557;
const int kMaxListen = 100;

int main(int argc, char** argv) {
  int ret, sfd, cfd, efd; 
  struct sockaddr_in serv_addr, clit_addr;
  socklen_t clit_len;
  char serv_ip[INET_ADDRSTRLEN];
  char clit_ip[INET_ADDRSTRLEN];

  memset(&serv_addr, 0, sizeof(serv_addr)); 
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(Port);

  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    perr_exit("socket", sfd);
  }

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


  if (inet_ntop(AF_INET, &serv_addr.sin_addr, serv_ip, sizeof(serv_ip)) == nullptr) {
    perr_exit("inet_ntop", errno);
  } 
  printf("begin server in %s:%d ...\n", serv_ip, Port);

  // struct epoll_event event;
  struct epoll_event revents[kMaxLen];
  struct epoll_event aevents[kMaxLen];

  efd = epoll_create(kMaxLen); 
  if (efd == -1) {
    perr_exit("epoll_create", efd);
  }

  aevents[0].events = EPOLLIN | EPOLLET;
  aevents[0].data.fd = sfd;
  ret = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &aevents[0]);

  for (int i = 1; i < kMaxLen; i++) {
    aevents[i].data.fd = -1;
  }

  if (ret == -1) {
    perr_exit("epoll_ctl", ret);
  }

  printf("begin listen ...\n");
  int res;
  for (;;) {
    res = epoll_wait(efd, revents, kMaxLen, -1);
    if (res == -1) {
      perr_exit("epoll_wait", ret);
    }

    if (revents[0].events == EPOLLIN) {
      memset(clit_ip, 0, sizeof(clit_ip));
      clit_len = sizeof(clit_addr);
      cfd = accept(sfd, reinterpret_cast<sockaddr*>(&clit_addr), &clit_len);
      if (cfd == -1) {
        perr_exit("accept", ret);
      }

      int i ;
      for (i = 1; i < kMaxLen; i++) {
        if (aevents[i].data.fd < 0) {
          aevents[i].data.fd = cfd;
          break;
        }
      }

      if (i == kMaxLen) {
        perr_exit("reach max", -1);
      }

      aevents[i].events = EPOLLIN | EPOLLET;
      ret = epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &aevents[i]);
      if (ret == -1) {
        perr_exit("epoll_ctl", ret);
      }
      if (inet_ntop(AF_INET, &clit_addr.sin_addr, clit_ip, sizeof(clit_ip)) == nullptr) {
        perr_exit("inet_ntop", errno);
      }
      int clit_port = ntohs(clit_addr.sin_port); 
      printf("accept client at %s:%d\n", clit_ip, clit_port);
    }

    for (int i = 0; i < res; i++) {
      if (revents[i].data.fd == sfd) { continue; }
      printf("event: %d\n", revents[i].events);
      if (revents[i].data.fd < 0) {
        continue;
      }

      cfd = revents[i].data.fd;    
      int n;
      int flag = fcntl(cfd, F_GETFL);
      flag |= O_NONBLOCK;
      flag = fcntl(cfd, F_SETFL, flag);
      if (flag == -1) {
        perr_exit("fcntl f_setfl", flag);
      }
      char buf[BUFSIZ];
      if ((n = read(cfd, buf, sizeof(buf))) == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          printf("read without data with noblock fd\n"); 
          continue;
        } else if (errno == ECONNRESET) {
          printf("client aborted connection\n");
          revents[i].data.fd = -1;
          close(cfd);
          continue;
        } else if (errno == EINTR) {
          printf("interupt read\n");
          continue;
        }
        perr_exit("read", n);
      } else if (n == 1) {
        close(cfd);
        revents[i].data.fd = -1;
        printf("client closed connection\n");
        continue;
      }

      for (int i = 0; i < n; i++) {
        buf[i] = toupper(buf[i]);
      }

      if ((n = write(cfd, buf, n)) == -1) {
        if (errno == EINTR) { continue; }
        perr_exit("client write", n);
      }
    }

  }

  return 0;
}
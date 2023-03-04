#include "net_utils.h"

#include <stdlib.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

void perr_exit(const char* s, int num=-1) {
  if (num == -1) {
    num = errno;
  }
  fprintf(stderr, "%s, errno=%d:%s\n", s, num, strerror(num));
  exit(1);
}

int my_accept(int fd, struct sockaddr* addr, socklen_t* len) {
  memset(addr, 0, sizeof(addr));
  int ret;
again:
  ret = accept(fd, addr, len);
  if (ret == -1) {
    if (errno == ECONNABORTED || errno == EINTR) { // 如果连接被终止或者别系统中断，就重新开始
      goto again;
    }
    perr_exit("accept", ret);
  }
  return ret;
}

int my_bind(int fd, struct sockaddr* addr, socklen_t len) {
  int ret;
  ret = bind(fd, addr, len);
  if (ret == -1) {
    perr_exit("bind", ret);
  }
  return ret;
}

int my_connect(int fd, struct sockaddr* addr, socklen_t len) {
  int ret;
  ret = connect(fd, addr, len);
  if (ret == -1) {
    perr_exit("connect", ret);
  }
  return ret;
}

int create_socket(int family, int type, int protocol) {
  int fd;
  fd = socket(family, type, protocol);
  if (fd == -1) {
    perr_exit("socket", fd);
  }
  return fd;
}

ssize_t my_read(int fd, void* ptr, size_t nbytes) {
  ssize_t n;
again:
  if ((n = read(fd, ptr, nbytes)) == -1) {
    if (errno == EINTR) { goto again; }
    perr_exit("read", n);
  }
  return n;
}

ssize_t my_write(int fd, const void* ptr, size_t nbytes) {
  ssize_t n;
again:
  if ((n = write(fd, ptr, nbytes)) == -1) {
    if (errno == EINTR) { goto again; }
    perr_exit("write", n);
  }
  return n;
}


ssize_t my_readn(int fd, void* vptr, size_t n) {
  int nleft = n;
  int ret;
  char* ptr = static_cast<char *>(vptr);
  while (nleft > 0) {
    if ((ret = read(fd, vptr, nleft)) == -1) {
      if (errno == EINTR) {
        ret = 0;
      } else {
        return -1;
      }  
    }
    nleft -= ret;
    ptr += ret;
  } 

  return n - nleft;
}

ssize_t my_wirten(int fd, const void* vptr, size_t n) {
  int nleft = n;
  int ret;
  const char* ptr = static_cast<const char*>(vptr);
  while (nleft > 0) {
    if ((ret = write(fd, vptr, nleft)) == -1) {
      if (errno == EINTR) {
        ret = 0;
      } else {
        return -1;
      }
    }
    nleft -= ret;
    ptr += nleft;
  }

  return n - nleft;
}

ssize_t my_readline(int fd, void* vptr, size_t len) {
  int ret;
  char* ptr = static_cast<char*>(vptr);
  char letter;
  for (int i = 0; i < len; i++) {
    if ((ret = read(fd, &letter, 1)) == -1) {
      if (errno == EINTR) {
        ret = 0;
      } else {
        return -1;
      }
    }
    len += ret;
    ptr[len] = letter;
    if (letter == '\n') {
      return len;
    }
  }
  fprintf(stderr, "%ld < sentence\n", len); 
  return ret;
}

int my_close(int fd) {
  int ret;
  ret = close(fd);
  if (ret == -1) {
    perr_exit("close", fd); 
  }
  return ret;
}

int my_listen(int fd, int maxnum) {
  int ret = listen(fd, maxnum);
  if (ret == -1) {
    perr_exit("listen", ret);
  }
  return ret;
}
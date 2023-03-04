#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <string.h>

void perr_exit(const char* s, int num=-1) {
  if (num == -1) {
    num = errno;
  }
  fprintf(stderr, "%s, errno = %d: %s\n", s, num, strerror(num));
  exit(1);
}

const int kMaxLen = 10;

int main(int argc, char** argv) {
  int ret;
  int pfd[2];
  ret = pipe(pfd);
  if (ret == -1) {
    perr_exit("pipe", ret);
  }

  int pid = fork();
  if (pid == -1) {
    perr_exit("pid", pid);
  }

  if (pid == 0) {
    char ch = 'a';
    close(pfd[0]);
    printf("I'am in child: %d ...\n", getpid());
    for (;;) {
      int i;
      char buf[kMaxLen]{0};
      for (i = 0; i < kMaxLen / 2; i++) {
        buf[i] = ch;
      }
      buf[i - 1] = '\n';
      ch++;

      for ( ; i < kMaxLen; i++) {
        buf[i] = ch;
      }
      buf[i - 1] = '\n';

      if ((ret = write(pfd[1], buf, kMaxLen)) <= 0) {
        if (errno == EINTR) {
          printf("again write\n");
          continue;
        }
      }
      ch++;
      sleep(3); 
    }
    close(pfd[1]);
  } else {
    close(pfd[1]);
    printf("I'am in parent: %d ...\n", getpid());
    struct epoll_event event;
    struct epoll_event resevent[10];
    int res, len, efd;
    event.events = EPOLLIN | EPOLLET; // 边缘触发 和 水平触发
    // event.events = EPOLLIN; // 水平触发
    efd = epoll_create(10);
    event.data.fd = pfd[0];
    ret = epoll_ctl(efd, EPOLL_CTL_ADD, pfd[0], &event);
    if (ret == -1) {
      perr_exit("epoll_ctl", ret);
    }
    char buf[kMaxLen]{0};
    for (;;) {
      res = epoll_wait(efd, resevent, 10, -1);
      if (ret == -1) {
        perr_exit("epoll_wait", ret);
      }
      printf("res: %d\n", res);
      if (resevent[0].data.fd == pfd[0]) {
        len = read(pfd[0], buf, kMaxLen / 2);
        write(STDOUT_FILENO, buf, len);
      }
    }
    
    close(pfd[0]);
    close(efd);
  } 

  return 0;
}
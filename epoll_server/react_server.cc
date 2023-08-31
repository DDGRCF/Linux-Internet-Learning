#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

const int Port = 12557;
const int kMaxEvents = 10;

using event_callback_t = void (*)(int fd, int events, void* arg);

struct myevent_s {
  int fd;
  int events;
  void* arg;
  event_callback_t call_back;
  int status;
  char buf[BUFSIZ];
  int len;
  long last_active;
};

int g_efd;
struct myevent_s g_events[kMaxEvents + 1];

void perr_exit(const char* s, int num = -1) {
  if (num == -1) {
    num = errno;
  }
  fprintf(stderr, "%s, errno = %d: %s\n", s, num, strerror(num));
  exit(1);
}

void set_nonblocking(int fd) {
  int flag = fcntl(fd, F_GETFL, 0);
  if (flag == -1) {
    perr_exit("fcntl f_getfl", flag);
  }
  flag = fcntl(fd, F_SETFL, flag);
  if (flag == -1) {
    perr_exit("fcntl f_setfl", flag);
  }
}

void eventset(struct myevent_s* ev, int fd, event_callback_t callback,
              void* arg) {
  ev->fd = fd;
  ev->call_back = callback;
  ev->arg = arg;
  ev->status = 0;
  memset(ev->buf, 0, sizeof(ev->buf));
  ev->len = 0;
  long last_active = time(nullptr);
}

void eventmod(int efd, int event, struct myevent_s* ev,
              event_callback_t callback) {
  struct epoll_event epv = {0, {0}};
  epv.data.ptr = ev;
  epv.events = ev->events = event;
  ev->call_back = callback;
  int ret;
  if ((ret = epoll_ctl(efd, EPOLL_CTL_MOD, ev->fd, &epv))) {
    perr_exit("epoll_ctl", ret);
  }
  if (ev->status == 0) {
    ev->status = 1;
  }
  printf("event change Ok fd=%d\n", ev->fd);
}

void eventadd(int efd, int event, struct myevent_s* ev) {
  struct epoll_event epv = {0, {0}};
  int opt, ret;
  epv.data.ptr = ev;
  epv.events = ev->events = event;

  if (ev->status == 0) {
    opt = EPOLL_CTL_ADD;
    ev->status = 1;
  }

  if ((ret = epoll_ctl(efd, opt, ev->fd, &epv)) == -1) {
    perr_exit("epoll_ctl", ret);
  }
  printf("event add Ok fd=%d\n", ev->fd);
}

void eventdel(int efd, struct myevent_s* ev) {
  int ret;
  struct epoll_event epv = {0, {0}};
  ev->status = 0;
  epv.data.ptr = nullptr;
  ret = epoll_ctl(efd, EPOLL_CTL_DEL, ev->fd, &epv);
  if (ret == -1) {
    perr_exit("epoll_ctl", ret);
  }
}

void send_callback(int lfd, int events, void* arg);

void recv_callback(int lfd, int events, void* arg) {
  int ret;
  char buf[BUFSIZ]{0};
  struct myevent_s* ev = static_cast<struct myevent_s*>(arg);
  printf("in recv_callback ...\n");
  while ((ret = read(lfd, buf, sizeof(buf))) == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      printf("read %d without data\n", lfd);
      continue;
    } else if (errno == EINTR) {
      printf("read %d being interrupted\n", lfd);
      continue;
    } else if (errno == ECONNRESET) {
      ev->status = 0;
      close(ev->fd);
      eventdel(g_efd, ev);
      printf("client close connection\n");
      break;
    } else {
      ev->status = 0;
      close(ev->fd);
      eventdel(g_efd, ev);
      perr_exit("read", ret);
    }
  }

  if (ret == 0) {
    ev->status = 0;
    close(ev->fd);
    eventdel(g_efd, ev);
    printf("client close connection\n");
    return;
  }

  printf("recv data: %s:%d\n", buf, ret);
  memcpy(ev->buf, buf, ret);
  ev->len = ret;

  // eventset(ev, lfd, send_callback, ev);
  eventmod(g_efd, EPOLLOUT, ev, send_callback);
}

void send_callback(int lfd, int events, void* arg) {
  int ret;
  char buf[BUFSIZ]{0};

  struct myevent_s* ev = static_cast<struct myevent_s*>(arg);
  memcpy(buf, ev->buf, ev->len);
  for (int i = 0; i < ev->len; i++) {
    buf[i] = toupper(buf[i]);
  }
  printf("get data: %s:%d\n", buf, ret);
  while ((ret = write(lfd, buf, ev->len)) == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      printf("write %d without data\n", lfd);
      continue;
    } else if (errno == EINTR) {
      printf("write %d being interrupted\n", lfd);
      continue;
    }
    perr_exit("write", ret);
  }

  if (ret == 0) {
    ev->status = 0;
    close(ev->fd);
    eventdel(g_efd, ev);
    return;
  }

  memset(ev->buf, 0, sizeof(ev->buf));
  ev->len = 0;
  eventmod(g_efd, EPOLLIN, ev, recv_callback);
}

void accept_callback(int lfd, int events, void* args) {
  int ret, cfd;
  char clit_ip[INET_ADDRSTRLEN]{0};
  struct sockaddr_in clit_addr;
  socklen_t clit_len;

  clit_len = sizeof(clit_addr);
  if ((cfd = accept(lfd, reinterpret_cast<sockaddr*>(&clit_addr), &clit_len)) ==
      -1) {
    perr_exit("accept", cfd);
  }

  if (inet_ntop(AF_INET, &clit_addr.sin_addr, clit_ip, sizeof(clit_ip)) ==
      nullptr) {
    perr_exit("inet_ntop");
  }
  int clit_port = ntohs(clit_addr.sin_port);
  printf("accept client from %s:%d\n", clit_ip, clit_port);

  int i;
  do {
    for (i = 0; i < kMaxEvents; i++) {
      if (g_events[i].status == 0) {
        break;
      }
    }
    if (i == kMaxEvents) {
      printf("%s: max connect limit[%d]\n", __func__, kMaxEvents);
      exit(1);
    }
    set_nonblocking(cfd);
    eventset(&g_events[i], cfd, recv_callback, &g_events[i]);
    eventadd(g_efd, EPOLLIN, &g_events[i]);
  } while (0);
}

void init_socket(int* efd, const int port, const int max_listen = 32) {
  int ret, sfd;
  char serv_ip[INET_ADDRSTRLEN];
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  *efd = epoll_create(kMaxEvents);
  if (*efd == -1) {
    perr_exit("epoll_create", *efd);
  }

  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    perr_exit("socket", sfd);
  }

  set_nonblocking(sfd);
  ret = bind(sfd, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr));
  if (ret == -1) {
    perr_exit("bind", ret);
  }

  ret = listen(sfd, max_listen);
  if (ret == -1) {
    perr_exit("listen", ret);
  }

  int opt = 1;
  ret = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (ret == -1) {
    perr_exit("setsockopt", ret);
  }

  eventset(&g_events[kMaxEvents], sfd, accept_callback, &g_events[kMaxEvents]);
  eventadd(*efd, EPOLLIN, &g_events[kMaxEvents]);

  if (inet_ntop(AF_INET, &serv_addr.sin_addr, serv_ip, sizeof(serv_ip)) ==
      nullptr) {
    perr_exit("inet_ntop");
  }

  printf("begin server at %s:%d ...\n", serv_ip, port);
}

int main() {
  init_socket(&g_efd, Port, kMaxEvents);
  struct epoll_event events[kMaxEvents + 1];
  int i;
  for (;;) {
    int nfd = epoll_wait(g_efd, events, kMaxEvents + 1, 1000);
    if (nfd < 0) {
      perr_exit("epoll_wait", nfd);
      break;
    }

    for (i = 0; i < nfd; i++) {
      struct myevent_s* ev = static_cast<struct myevent_s*>(events[i].data.ptr);
      if ((events[i].events & EPOLLIN) && (ev->events & EPOLLIN)) {
        ev->call_back(ev->fd, events[i].events, ev->arg);
      }
      if ((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT)) {
        ev->call_back(ev->fd, events[i].events, ev->arg);
      }
    }
  }
}

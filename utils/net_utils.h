#ifndef NET_UTILS_H_
#define NET_UTILS_H_

#include <sys/socket.h>

void perr_exit(const char* s, int num);

int my_accept(int fd, struct sockaddr* addr, socklen_t* len);

int my_bind(int fd, struct sockaddr* addr, socklen_t len);

int my_listen(int fd, int maxnum);

int my_connect(int fd, struct sockaddr* addr, socklen_t len);

int create_socket(int family, int type, int protocol);

ssize_t my_read(int fd, void* ptr, size_t nbytes);

ssize_t my_write(int fd, const void* ptr, size_t nbytes);


ssize_t my_readn(int fd, void* vptr, size_t n);

ssize_t my_wirten(int fd, const void* vptr, size_t n);

ssize_t my_readline(int fd, void* vptr, size_t len);

int my_close(int fd);

#endif
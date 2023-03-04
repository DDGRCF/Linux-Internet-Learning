#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

void say_error(const char* msg, int num) {
  fprintf(stderr, "%s, errno=%d:%s", msg, num, strerror(num));
  exit(1);
}

int main() {
  // inet_ntoa
  int ret;
  const char* ip = "192.168.0.12";
  struct in_addr addr;
  addr.s_addr = inet_addr(ip); // 这里给必须是in_addr_t类型的
  char* ip_str = inet_ntoa(addr);
  printf("IP address: %s\n", ip_str);
  // inet_aton
  ret = inet_aton(ip, &addr);
  if (ret != 1) {
    say_error("inet_aton", ret);
  }
  printf("IP address: %u\n", ntohl(addr.s_addr));
  // inet_ntop
  const char* ip2 = "192.168.0.15";
  char ip_str_buf[INET_ADDRSTRLEN];
  addr.s_addr = inet_addr(ip2);
  if (inet_ntop(AF_INET, &addr, ip_str_buf, sizeof(ip_str_buf)) == nullptr) {
    say_error("inet_ntop", ret);
  }

  printf("IP address: %s\n", ip_str_buf);
  // inet_pton
  ret = inet_pton(AF_INET, ip2, &addr);
  if (ret != 1) {
    say_error("inet_pton", ret);
  }
  printf("IP address: %u\n", ntohl(addr.s_addr));

  return 0;
}
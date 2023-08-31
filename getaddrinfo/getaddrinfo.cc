#include <errno.h>
#include <ifaddrs.h>
#include <memory.h>
#include <netdb.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>

using namespace std;

int main() {
  const char* address = "www.baidu.com";
  uint16_t port = 22;

  addrinfo hints, *results;
  bzero(&hints, sizeof(hints));

  hints.ai_flags = AI_NUMERICHOST;
  hints.ai_family = AF_UNSPEC;

  int ret = getaddrinfo(address, nullptr, &hints, &results);

  if (ret) {
    fprintf(stderr, "getaddrinfo error, code: %d, msg: %s\n", ret,
            strerror(errno));
    exit(1);
  }

  char ip[INET_ADDRSTRLEN]{0};

  inet_ntop(AF_INET, reinterpret_cast<sockaddr_in*>(results->ai_addr), ip,
            sizeof(ip));

  cout << ip << endl;

  freeaddrinfo(results);
  return 0;
}

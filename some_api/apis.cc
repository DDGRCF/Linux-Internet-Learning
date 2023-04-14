#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  struct in_addr addr;
  inet_aton("8.8.8.8", &addr);

  struct hostent *host = gethostbyaddr(&addr, sizeof(addr), AF_INET);

  if (host == NULL) {
    printf("gethostbyaddr failed\n");
    return 1;
  }

  printf("Official name: %s\n", host->h_name);

  for (char **p = host->h_aliases; *p != NULL; ++p) {
    printf("Alias: %s\n", *p);
  }

  for (char **p = host->h_addr_list; *p != NULL; ++p) {
    printf("Address: %s\n", inet_ntoa(*(struct in_addr *)*p));
  }

  return 0;
}
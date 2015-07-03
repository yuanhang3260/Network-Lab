#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>           // close()
#include <string.h>           // strcpy, memset()

#include <netinet/ip.h>       // IP_MAXPACKET (65535)
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t
#include <sys/socket.h>       // needed for socket()
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

int ConfigureIPHeaderInclude(int fd) {
  int on = 1;
  if (setsockopt(fd, IPPROTO_IPV6, IP_HDRINCL, &on, sizeof(on)) < 0) {
    fprintf(stderr, "set socket option IP_HDRINCL failed\n");
    return -1;
  }
  return 0;
}

int ConfigureSocketPriority(int fd, int priority) {
  if (setsockopt(fd, SOL_SOCKET, SO_PRIORITY,
                 &priority, sizeof(priority)) < 0) {
    fprintf(stderr, "set socket option SO_PRIORITY failed\n");
    return -1;
  }
  return 0;
}

int ConfigureSendMulticastLoop(int fd, int x) {
  if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
                 &x, sizeof(x)) < 0) {
    fprintf(stderr, "set socket option IPV6_MULTICAST_LOOP failed\n");
    return -1;
  }
  return 0;
}

int ConfigureSendBroadcast(int fd, int x) {
  if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST,
                 &x, sizeof(x)) < 0) {
    fprintf(stderr, "set socket option SO_BROADCAST failed\n");
    return -1;
  }
  return 0;
}

int main(int argc, char** argv) {
  int proto = 112;
  if (argc > 1) {
    proto = atoi(argv[1]);
  }
  printf("proto is %d\n", proto);
  
  int family = AF_INET6;

  int send_fd = socket(family, SOCK_RAW, proto);
  if (send_fd < 0) {
    fprintf(stderr, "create send socket failed\n");
    return -1;
  }

  // ConfigureIPHeaderInclude(send_fd);
  ConfigureSocketPriority(send_fd, 0);
  ConfigureSendMulticastLoop(send_fd, 1);
  ConfigureSendBroadcast(send_fd, 1);

  char buf[32];
  strncpy(buf, "hello\0", 6);

  sockaddr_in6 dest;
  memset(&dest, 0, sizeof(dest));
  dest.sin6_family = family;

  inet_pton(AF_INET6, "ff15::1", &dest.sin6_addr);
  // inet_pton(AF_INET6, "fe80::62f8:1dff:fec7:a6fc", &dest.sin6_addr);
  // inet_aton("127.0.0.1", &dest.sin_addr);
  int nsent = sendto(send_fd, buf, 6, 0,
                     reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
  if (nsent < 0) {
    fprintf(stderr, "send raw packet failed\n");
    return -1;
  }

  return 0;
}

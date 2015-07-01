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

  // int on = 1;
  // if (setsockopt(send_fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
  //   fprintf(stderr, "set socket option IP_HDRINCL failed\n");
  //   return -1;
  // }
  
  int socket_priority = 0;
  if (setsockopt(send_fd, SOL_SOCKET, SO_PRIORITY,
                 &socket_priority, sizeof(socket_priority)) < 0) {
    fprintf(stderr, "set socket option SO_PRIORITY failed\n");
    return -1;
  }

  // uint32_t x = 1;
  // if (setsockopt(send_fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
  //                &x, sizeof(x)) < 0) {
  //   fprintf(stderr, "set socket option IPV6_MULTICAST_LOOP failed\n");
  //   return -1;
  // }

  int is_on = 1;
  if (setsockopt(send_fd, SOL_SOCKET, SO_BROADCAST,
                 &is_on, sizeof(is_on)) < 0) {
    fprintf(stderr, "set socket option SO_BROADCAST failed\n");
    return -1;
  }

  char buf[32];
  strncpy(buf, "hello\0", 6);

  sockaddr_in6 dest;
  memset(&dest, 0, sizeof(dest));
  dest.sin6_family = family;
  
  inet_pton(AF_INET6, "ff02::12", &dest.sin6_addr);
  // dest.sin6_addr.s6_addr[15] = 0x1;
  // inet_aton("127.0.0.1", &dest.sin_addr);
  int nsent = sendto(send_fd, buf, 6, 0,
                     reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
  if (nsent < 0) {
    fprintf(stderr, "send raw packet failed\n");
    return -1;
  }

  return 0;
}

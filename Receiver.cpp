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
#include <net/if.h>

#include <iostream>

int ConfigureReceiveMulticast(
    int fd,
    bool join_multicast_group,
    const char* ip,
    const char* dev) {
  
  in6_addr multicast_ip;
  inet_pton(AF_INET6, ip, &multicast_ip);

  struct ipv6_mreq req;

  memset(&req, 0, sizeof(req));

  memmove(&req.ipv6mr_multiaddr, &multicast_ip, sizeof(req.ipv6mr_multiaddr));
  req.ipv6mr_interface = if_nametoindex(dev);

  int op = join_multicast_group ? IPV6_ADD_MEMBERSHIP : IPV6_DROP_MEMBERSHIP;
  if (setsockopt(fd, IPPROTO_IPV6, op, &req, sizeof(req))) {
    fprintf(stderr, "change multicast group failed\n");
    return -1;
  }
  return 0;
}

int ConfigureReceiveTimeout(int fd, int time) {
  struct timeval tv;
  tv.tv_usec = 0;
  tv.tv_sec = time;
  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    fprintf(stderr, "set socket option SO_RCVTIMEO failed\n");
    return -1;
  }
  return 0;
}

int ConfigureBindToDevice(int fd, const char* dev) {
  if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE,
                 dev, strlen(dev) + 1) < 0) {
    fprintf(stderr, "set socket option SO_BINDTODEVICE failed\n");
    return -1;
  }
  return 0;
}

int main(int argc, char** argv) {

  int proto = 112;
  if (argc > 1) {
    proto = atoi(argv[1]);
  }

  const char* dev_name = "wlan0";

  int recv_fd = socket(AF_INET6, SOCK_RAW, proto);
  if (recv_fd < 0) {
    fprintf(stderr, "create receive socket failed\n");
    return -1;
  }

  ConfigureBindToDevice(recv_fd, dev_name);
  ConfigureReceiveMulticast(recv_fd, true, "ff15::01", dev_name);
  ConfigureReceiveTimeout(recv_fd, 60);

  char buf[4096];
  while (1) {
    int nrecv = recv(recv_fd, buf, sizeof(buf), 0);
    if (nrecv < 0) {
      fprintf(stderr, "receive raw packet failed\n");
      return -1;
    }

    std::cout << "Received " << nrecv << " bytes" << std::endl;
    // struct iphdr* ip_header = reinterpret_cast<struct iphdr*>(buf);
    // std::cout << "src addr "
    //           << inet_ntoa(*reinterpret_cast<in_addr*>(&ip_header->saddr))
    //           << std::endl;
    // std::cout << "dst addr "
    //           << inet_ntoa(*reinterpret_cast<in_addr*>(&ip_header->daddr))
    //           << std::endl;
    // printf("protocol %x\n", *reinterpret_cast<uint8_t*>(&ip_header->protocol));
    // std::cout << "src port "
    //           << ntohs(*reinterpret_cast<unsigned short*>(buf + sizeof(struct iphdr)))
    //           << std::endl;
    // buf[nrecv] = '\0';
    // std::cout << "content: "
    //           << std::string(buf + sizeof(struct iphdr) + 20, nrecv - 40)
    //           << std::endl;
    std::cout << "content: "
              << std::string(buf, nrecv)
              << std::endl;
  }
  return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>           // close()
#include <string.h>           // strcpy, memset()
#include <netinet/ip.h>       // IP_MAXPACKET (65535)
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t
#include <sys/socket.h>       // needed for socket()
#include <linux/if_ether.h>   // ETH_P_ARP = 0x0806, ETH_P_ALL = 0x0003
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

#include <iostream>

int main(int argc, char** argv) {
  int proto = 112;
  if (argc > 1) {
    proto = atoi(argv[1]);
  }
  printf("proto is %d\n", proto);

  int recv_fd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
  if (recv_fd < 0) {
    fprintf(stderr, "create receive socket failed\n");
    return -1;
  }

  char buf[4096];
  while (1) {
    int nrecv = recv(recv_fd, buf, sizeof(buf), 0);
    if (nrecv < 0) {
      fprintf(stderr, "receive raw packet failed\n");
      return -1;
    }

    std::cout << "Received " << nrecv << " bytes" << std::endl;
    struct iphdr* ip_header = reinterpret_cast<struct iphdr*>(buf);
    std::cout << "src addr "
              << inet_ntoa(*reinterpret_cast<in_addr*>(&ip_header->saddr))
              << std::endl;
    std::cout << "dst addr "
              << inet_ntoa(*reinterpret_cast<in_addr*>(&ip_header->daddr))
              << std::endl;
    printf("protocol %x\n", *reinterpret_cast<uint8_t*>(&ip_header->protocol));
    std::cout << "src port "
              << ntohs(*reinterpret_cast<unsigned short*>(buf + sizeof(struct iphdr)))
              << std::endl;
    buf[nrecv] = '\0';
    std::cout << "content: "
              << std::string(buf + sizeof(struct iphdr) + 20, nrecv - 40)
              << std::endl;
    // std::cout << "content: "
    //           << std::string(buf, nrecv)
    //           << std::endl;
  }
  return 0;
}

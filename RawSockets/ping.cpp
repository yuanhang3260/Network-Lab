#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* calcsum - used to calculate IP and ICMP header checksums using
 * one's compliment of the one's compliment sum of 16 bit words of the header
 */
unsigned short calcsum(unsigned short *buffer, int length)
{
  unsigned long sum;  

  // initialize sum to zero and loop until length (in words) is 0 
  for (sum=0; length>1; length-=2) // sizeof() returns number of bytes, we're interested in number of words 
    sum += *buffer++; // add 1 word of buffer to sum and proceed to the next 

  // we may have an extra byte 
  if (length==1)
    sum += (char)*buffer;

  sum = (sum >> 16) + (sum & 0xFFFF);  // add high 16 to low 16 
  sum += (sum >> 16);        // add carry 
  return ~sum;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s dest_ip\n", argv[0]);
    return -1;
  }

  int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (fd < 0) {
    fprintf(stderr, "ERROR: create socket faied\n");
    return -1;
  }

  struct icmphdr icmp_hdr;

  // clear out the packet, and fill with contents.
  memset(&icmp_hdr, 0, sizeof(struct icmphdr));
  icmp_hdr.type = ICMP_ECHO;
  icmp_hdr.un.echo.sequence = 50;  // just some random number.
  icmp_hdr.un.echo.id = 48;        // just some random number.
  icmp_hdr.checksum = calcsum((unsigned short*)&icmp_hdr,
                               sizeof(struct icmphdr));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  inet_aton(argv[1], &addr.sin_addr);

  // finally, send the packet.
  int rc = sendto(fd,
                  &icmp_hdr,
                  sizeof(struct icmphdr),
                  0, // flags
                  (struct sockaddr*)&addr,
                  sizeof(addr));
  if (rc == -1) {
    fprintf(stderr, "ERROR: Send ICMP failed\n");
    return -1;
  }

  // Receive echo reply
  // The received packet contains the IP header.
  char rbuf[sizeof(struct iphdr) + sizeof(struct icmp)];
  struct sockaddr_in raddr;
  socklen_t raddr_len;

  // Receive the packet that we sent (since we sent it to ourselves,
  // and a raw socket sees everything...).
  rc = recvfrom(fd,
                rbuf, sizeof(rbuf),
                0, // flags
                (struct sockaddr*)&raddr, &raddr_len);
  if (rc == -1) {
    fprintf(stderr, "ERROR: Receive ICMP reply failed\n");
    return -1;
  }

  struct iphdr* iphdr = NULL;
  struct icmphdr* recv_icmphdr = NULL;

  // we got an IP packet - verify that it contains an ICMP message.
  iphdr = (struct iphdr*)rbuf;
  if (iphdr->protocol != IPPROTO_ICMP) {
      fprintf(stderr, "Expected ICMP packet, got %u\n", iphdr->protocol);
      return -1;
  }

  // verify that it's an ICMP echo request, with the expected seq. num + id.
  recv_icmphdr = (struct icmphdr*)(rbuf + (iphdr->ihl * 4));
  if (recv_icmphdr->type != ICMP_ECHOREPLY) {
      fprintf(stderr, "Expected ICMP echo-reply, got %u\n", recv_icmphdr->type);
      return -1;
  }
  if (recv_icmphdr->un.echo.sequence != 50) {
      fprintf(stderr,
              "Expected sequence 50, got %d\n", recv_icmphdr->un.echo.sequence);
      return -1;
  }
  if (recv_icmphdr->un.echo.id != 48) {
      fprintf(stderr,
              "Expected id 48, got %d\n", recv_icmphdr->un.echo.id);
      return -1;
  }
  printf("Got the expected ICMP echo-request\n");

  return 0;
}

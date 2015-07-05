#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

char* hostname = 0;
int packets_sent = 0;
int packets_received = 0;

/* calcsum - used to calculate IP and ICMP header checksums using
 * one's compliment of the one's compliment sum of 16 bit words of the header
 */
static int in_cksum(unsigned short *buf, int sz) {
  int nleft = sz;
  int sum = 0;
  unsigned short *w = buf;
  unsigned short ans = 0;

  while (nleft > 1) {
    sum += *w++;
    nleft -= 2;
  }

  if (nleft == 1) {
    *(unsigned char *) (&ans) = *(unsigned char *) w;
    sum += ans;
  }
   
  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  ans = ~sum;
  return (ans);
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

void sig_handler(int signo) {
  if (signo == SIGINT) {
    printf("\n--- %s ping statistics ---\n", hostname);
    printf("%d packets trasmitted, %d received, %d%% packet loss\n",
           packets_sent,
           packets_received,
           100 - packets_received * 100 / packets_sent);
  }
  exit(0);
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s dest_ip\n", argv[0]);
    return -1;
  }
  hostname = argv[1];

  if (signal(SIGINT, sig_handler) == SIG_ERR) {
    printf("ERROR: Can't catch SIGINT\n");
  }

  struct hostent *h;
  if (!(h = gethostbyname(hostname))) {
    fprintf(stderr, "ping: unknown host %s\n", hostname);
    return -1;
  }

  int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (fd < 0) {
    fprintf(stderr, "ERROR: create socket faied\n");
    return -1;
  }

  ConfigureReceiveTimeout(fd, 3);

  char buf[sizeof(struct icmphdr) + 32];
  struct icmphdr* icmp_hdr = (struct icmphdr*)buf;
  int sequence = 1;
  printf("PING %s (%s) %d(%d) bytes of data.\n",
         hostname,
         inet_ntoa(*((struct in_addr*)h->h_addr)),
         56,
         84);

  while (1) {
    // clear out the packet, and fill with contents.
    memset(icmp_hdr, 0, sizeof(struct icmphdr));
    icmp_hdr->type = ICMP_ECHO;
    icmp_hdr->un.echo.sequence = sequence++;  // just some random number.
    icmp_hdr->un.echo.id = 5;        // just some random number.
    buf[sizeof(struct icmphdr) + 0] = 't';
    buf[sizeof(struct icmphdr) + 1] = 'e';
    buf[sizeof(struct icmphdr) + 2] = 's';
    buf[sizeof(struct icmphdr) + 3] = 't';
    icmp_hdr->checksum = in_cksum((unsigned short*)icmp_hdr,
                                  sizeof(buf));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr, h->h_addr, sizeof(addr.sin_addr));
  
    // finally, send the packet.
    int rc = sendto(fd,
                    buf,
                    sizeof(buf),
                    0, // flags
                    (struct sockaddr*)&addr,
                    sizeof(addr));
    packets_sent++;
    if (rc == -1) {
      fprintf(stderr, "ERROR: Send ICMP failed\n");
      sleep(1);
      continue;
    }

    // Receive echo reply
    // The received packet contains the IP header.
    char rbuf[sizeof(struct iphdr) + sizeof(struct icmp) + 256];
    struct sockaddr_in raddr;
    socklen_t raddr_len = sizeof(raddr);

    // Receive the packet that we sent (since we sent it to ourselves,
    // and a raw socket sees everything...).
    rc = recvfrom(fd,
                  rbuf,
                  sizeof(rbuf),
                  0, // flags
                  (struct sockaddr*)&raddr,
                  &raddr_len);
    if (rc == -1) {
      fprintf(stderr, "ERROR: Receive ICMP reply failed\n");
      sleep(1);
      continue;
    }

    struct iphdr* iphdr = NULL;
    struct icmphdr* recv_icmphdr = NULL;

    // we got an IP packet - verify that it contains an ICMP message.
    iphdr = (struct iphdr*)rbuf;
    if (iphdr->protocol != IPPROTO_ICMP) {
      fprintf(stderr, "Expected ICMP packet, got %u\n", iphdr->protocol);
      sleep(1);
      continue;
    }

    // verify that it's an ICMP echo request, with the expected seq. num + id.
    recv_icmphdr = (struct icmphdr*)(rbuf + (iphdr->ihl * 4));
    if (recv_icmphdr->type != ICMP_ECHOREPLY) {
      fprintf(stderr, "Expected ICMP echo-reply, got %u\n", recv_icmphdr->type);
      sleep(1);
      continue;
    }
    if (recv_icmphdr->un.echo.sequence != sequence - 1) {
      fprintf(stderr,
              "Expected sequence %d, got %d\n", 
              sequence - 1,
              recv_icmphdr->un.echo.sequence);
      sleep(1);
      continue;
    }
    if (recv_icmphdr->un.echo.id != 5) {
      fprintf(stderr,
              "Expected id 5, got %d\n", recv_icmphdr->un.echo.id);
      sleep(1);
      continue;
    }

    packets_received++;
    printf("%d bytes from %s: icmp_seq = %d ttl = %d\n",
           rc,
           inet_ntoa(*((struct in_addr*)h->h_addr)),
           recv_icmphdr->un.echo.sequence,
           iphdr->ttl
           );
    fprintf(stderr, "received data: ");
    int i = 0;
    for (i = 0; i < 32; i++) {
      char c = rbuf[sizeof(struct iphdr) + sizeof(struct icmphdr) + i];
      if (isascii(c)) {
        fprintf(stderr, "%c", c);
      } else {
        fprintf(stderr, ".");
      }
    }
    fprintf(stderr, "\n");
    sleep(1);
  }
  return 0;
}

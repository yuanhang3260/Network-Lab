#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/signal.h>
#include <string.h>
#include <netdb.h>

extern int h_errno;

#define DEFDATALEN      56
#define MAXIPLEN        60
#define MAXICMPLEN      76
 
static char *hostname = NULL;
 
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
 
static void noresp(int ign) {
  printf("No response from %s\n", hostname);
  exit(0);
}

static void ping(const char *host) {
  struct hostent *h;
  struct sockaddr_in pingaddr;
  struct icmphdr *pkt;
  int pingsock, c;
  char packet[DEFDATALEN + MAXIPLEN + MAXICMPLEN];

  if ((pingsock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
    perror("ping: creating a raw socket");
    exit(1);
  }

  // drop root privs if running setuid
  setuid(getuid());

  // listen for replies
  for (int ii = 0; ii < 10; ii++) {
    memset(packet, 0, sizeof(packet));
    memset(&pingaddr, 0, sizeof(struct sockaddr_in));

    pingaddr.sin_family = AF_INET;
    if (!(h = gethostbyname(host))) {
      fprintf(stderr, "ping: unknown host %s\n", host);
      exit(1);
    }
    memcpy(&pingaddr.sin_addr, h->h_addr, sizeof(pingaddr.sin_addr));
    hostname = h->h_name;

    pkt = (struct icmphdr *) packet;
    pkt->type = ICMP_ECHO;
    pkt->un.echo.sequence = 50;  // just some random number.
    pkt->un.echo.id = 48;        // just some random number.
    *(packet + sizeof(struct icmp)) = 't';
    *(packet + sizeof(struct icmp) + 1) = 'e';
    *(packet + sizeof(struct icmp) + 2) = 's';
    *(packet + sizeof(struct icmp) + 3) = 't';
    pkt->checksum = in_cksum((unsigned short *) pkt, sizeof(packet));

    c = sendto(pingsock, packet, sizeof(struct icmphdr), 0,
               (struct sockaddr *) &pingaddr, sizeof(struct sockaddr_in));

    if (c < 0 || c != sizeof(struct icmphdr)) {
      if (c < 0) {
        perror("ping: sendto");
      }
      fprintf(stderr, "ping: write incomplete\n");
      exit(1);
    }

    //signal(SIGALRM, noresp);
    //alarm(2);   // give the host 5000ms to respond
    struct sockaddr_in from;
    size_t fromlen = sizeof(from);

    if ((c = recvfrom(pingsock, packet, sizeof(packet), 0,
                      (struct sockaddr *) &from, &fromlen)) < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("ping: recvfrom");
      continue;
    }
    printf("received %d bytes\n", c);
    if (c >= 0 /* sizeof(struct iphdr) + sizeof(struct icmp)*/) {  // ip + icmp
      struct iphdr *iphdr = (struct iphdr *) packet;

      struct icmphdr* icmp_hdr = (struct icmphdr *) (packet + (iphdr->ihl << 2));  // skip ip hdr
      printf("id = %d\n", icmp_hdr->un.echo.id);
      printf("sequence = %d\n", icmp_hdr->un.echo.sequence);
      printf("data: \n");
      for (char* c = (char*)icmp_hdr + sizeof(struct icmphdr); c < packet + sizeof(packet); c++) {
        printf("%x", *c);
      }
      printf("\n");
      if (icmp_hdr->type != ICMP_ECHOREPLY) {
        break;
      }
    }
  }
  printf("%s is alive!\n", hostname);
  return;
}

int main (int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s dest_ip\n", argv[0]);
    return -1;
  }

  ping(argv[1]);
  return 0;
}

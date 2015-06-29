CC=g++ -std=c++11
CFLAGS=-Wall -Werror -O2

all: receiver sender arp_receiver

receiver: Receiver.cpp
		$(CC) $(CFLAGS) Receiver.cpp -o receiver

sender: Sender.cpp
		$(CC) $(CFLAGS) Sender.cpp -o sender

arp_receiver: ARP_Receiver.cpp
		$(CC) $(CFLAGS) ARP_Receiver.cpp -o arp_receiver

clean:
	rm -rf receiver sender arp_receiver
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/if_ether.h>

void proces_ethernet(unsigned char *buf,int data_size);

void proces_ip4(unsigned char *buf,int data_size);

void proces_ip6(unsigned char *buf,int data_size);

void proces_arp(unsigned char *buf,int data_size);

void proces_tcp(unsigned char *buf,int data_size);

void proces_udp(unsigned char *buf,int data_size);

void proces_xtp(unsigned char *buf,int data_size);

void proces_snp(unsigned char *buf,int data_size);

int exit_with_perror(char *msg) 
{
    perror(msg);
    exit(0);
}

void proces_packet(unsigned char *buf,int data_size)
{
	proces_ethernet(buf,data_size);
}

void proces_ethernet(unsigned char *buf,int data_size)
{
	struct ether_header *read_ether = (struct ether_header*)buf;
	
	printf("\nEthernet:\n");

	/* printing a destination mac addres */
	printf("Destination MAC addres: %02x:%02x:%02x:%02x:%02x:%02x\n",
			read_ether -> ether_dhost[0],
			read_ether -> ether_dhost[1],
		       	read_ether -> ether_dhost[2],
		       	read_ether -> ether_dhost[3],
		       	read_ether -> ether_dhost[4],
			read_ether -> ether_dhost[5]);	
	
	/* printing a source mac addres */
	printf("Source  MAC addres: %02x:%02x:%02x:%02x:%02x:%02x\n",
			read_ether -> ether_shost[0],
			read_ether -> ether_shost[1],
		       	read_ether -> ether_shost[2],
		       	read_ether -> ether_shost[3],
		       	read_ether -> ether_shost[4],
			read_ether -> ether_shost[5]);	
	
	/* printing a ethertype */
	printf("Ethertype: %#06hx\n",ntohs(read_ether -> ether_type));	
	
	/* procesing IPv4 or IPv6 or ARP or skips */
	switch(ntohs(read_ether -> ether_type))
	{
		case 0x0800:
		      proces_ip4(buf,data_size);
		      break;

		case 0x86DD:
		      proces_ip6(buf,data_size);
		      break;

		case 0x0806:
		      proces_arp(buf,data_size);
		      break;

		default:
		      printf("Unsupported protocol\n\n");
		      break;
	}
}


void proces_ip4(unsigned char *buf,int data_size)
{
	struct ip *read_ip = (struct ip*)buf;
	printf("\nIPv4:\n");
	printf("Header lenght: %d\nVersion: %d\n",read_ip -> ip_hl,read_ip -> ip_v);
	printf("Checksum: %d\nIdentification: %d\n",read_ip -> ip_sum,read_ip -> ip_id);
	printf("Destination address: %x\n",read_ip -> ip_dst);
	printf("Source address: %x\n",read_ip -> ip_src);
	printf("Time to live: %d\nNext protocol: %d\n",read_ip -> ip_ttl,read_ip -> ip_p);
	
	switch(read_ip -> ip_p)
	{
		case 6:
			proces_tcp(buf,data_size);
			break;
		case 17:
			proces_udp(buf,data_size);
			break;
		case 36:
			proces_xtp(buf,data_size);	
			break;
		case 109:
			proces_snp(buf,data_size);
			break;
		default:
			printf("Unsupported protocol\n");
			break;	
	}
}
void proces_ip6(unsigned char *buf,int data_size)
{
	struct ip6_hdr *read_ip6 = (struct ip6_hdr*)buf;
	printf("\nIPv6:\n");
	printf("Next protocol: %d\n",read_ip6 -> ip6_ctlun.ip6_un1.ip6_un1_nxt);
	printf("Hop limit:  %d\n",read_ip6 -> ip6_ctlun.ip6_un1.ip6_un1_hlim);
	printf("Payload lenght: %d\n",read_ip6 -> ip6_ctlun.ip6_un1. ip6_un1_plen);
	printf("Destination address: %s\n",read_ip6 -> ip6_dst.s6_addr);
	printf("Source address: %s\n",read_ip6 -> ip6_src.s6_addr);

	switch(read_ip6 ->  ip6_ctlun.ip6_un1.ip6_un1_nxt)
	{
		case 6:
			proces_tcp(buf,data_size);
			break;
		case 17:
			proces_udp(buf,data_size);
			break;
		case 36:
			proces_xtp(buf,data_size);	
			break;
		case 109:
			proces_snp(buf,data_size);
			break;
		default:
			printf("Unsupported protocol\n");
			break;	
	}
}

void proces_arp(unsigned char *buf,int data_size)
{
	struct ether_arp *read_arp = (struct ether_arp*)buf;
	printf("\nARP:\n");
	printf("Destination hardware address: %#02x:%#02x:%#02x:%#02x:%#02x:%#x\n",
			read_arp -> arp_tha[0],
			read_arp -> arp_tha[1],
			read_arp -> arp_tha[2],	
			read_arp -> arp_tha[3],
			read_arp -> arp_tha[4],
			read_arp -> arp_tha[5]
			);
	printf("Source hardware address: %#02x:%#02x:%#02x:%#02x:%#02x:%#02x\n",
			read_arp -> arp_sha[0],
			read_arp -> arp_sha[1],
			read_arp -> arp_sha[2],	
			read_arp -> arp_sha[3],
			read_arp -> arp_sha[4],
			read_arp -> arp_sha[5]
			);
	printf("Destination protocol address: %#02x:%#02x:%#02x:%#02x\n",
			read_arp -> arp_tpa[0],
			read_arp -> arp_tpa[1],
			read_arp -> arp_tpa[2],
			read_arp -> arp_tpa[3]
	     		);
	printf("Source protocol address: %#02x:%#02x:%#02x:%#02x\n",	
			read_arp -> arp_spa[0],
			read_arp -> arp_spa[1],
			read_arp -> arp_spa[2],
			read_arp -> arp_spa[3]
			);
}

void proces_tcp(unsigned char *buf,int data_size)
{
	struct tcphdr *read_tcp = (tcphdr*)buf;
	printf("\nTCP:\n");
	printf("Destination port: %d\n",read_tcp -> th_sport);
}

void proces_udp(unsigned char *buf,int data_size)
{
	struct udphdr *read_udp = (udphdr*)buf;
	printf("\nUDP:\n");
}

void proces_xtp(unsigned char *buf,int data_size)
{
	printf("\nXTP:\n");
}

void proces_snp(unsigned char *buf,int data_size)
{
	printf("\nSNP:\n");
}

int main(void)
{

	int sd,data_size;	
	unsigned char *buf = (unsigned char *)malloc(65536);	
	
	/* creating file descriptor from socket */	
	if( (sd = socket( AF_PACKET, SOCK_RAW, htons(ETH_P_ALL) ) ) == -1)
		exit_with_perror("socket\n");
	
	/* listening */
	for(;;)
	{	
		if( (data_size = recv(sd,buf,65536,0)) == -1 )	
			exit_with_perror("recv\n");
		
		proces_packet(buf,data_size);
	}

	close(sd);
	return 0;
}

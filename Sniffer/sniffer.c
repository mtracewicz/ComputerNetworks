#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/if_ether.h>

struct dhcpmsg
{
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    char chaddr[16];
    char sname[64];
    char file[128];
    char magic[4];
    char opt[3];
};

int  exit_with_perror(char *msg);

void proces_packet(unsigned char *buf,int data_size);

void proces_ethernet(unsigned char *buf,int data_size);

void proces_ip4(unsigned char *buf,int data_size);

void proces_ip6(unsigned char *buf,int data_size);

void select_next_protocol(unsigned char *buf,int data_size,int next_protocol);

void proces_arp(unsigned char *buf,int data_size);

void proces_tcp(unsigned char *buf,int data_size);

void proces_udp(unsigned char *buf,int data_size);

void tcp_udp_ports(unsigned char *buf,int data_size,unsigned short port);	

void proces_echo(unsigned char *buf,int data_size);

void proces_ftp(unsigned char *buf,int data_size);

void proces_http(unsigned char *buf,int data_size);

void proces_dhcp(unsigned char *buf,int data_size);

void proces_xtp(unsigned char *buf,int data_size);

void proces_snp(unsigned char *buf,int data_size);

void print_data(unsigned char *buf,int data_size);

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
	
	buf +=  sizeof(struct ether_header);
	data_size  -=  sizeof(struct ether_header);
	
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
	printf("Destination address: %s\n",inet_ntoa(read_ip->ip_dst));
	printf("Source address: %s\n",inet_ntoa(read_ip->ip_src));
	printf("Total length: %d\n",read_ip ->ip_len);
	printf("Time to live: %d\nNext protocol: %d\n",read_ip -> ip_ttl,read_ip -> ip_p);

	buf += 4 * read_ip -> ip_hl;	
	data_size -= 4 * read_ip -> ip_hl;	
	
	select_next_protocol(buf,data_size,read_ip->ip_p);
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

	buf += sizeof(struct ip6_hdr);
	data_size -= sizeof(struct ip6_hdr);
	
	select_next_protocol(buf,data_size,read_ip6 ->  ip6_ctlun.ip6_un1.ip6_un1_nxt);
}

void select_next_protocol(unsigned char *buf,int data_size,int next_protocol)
{
	switch(next_protocol)
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
	struct tcphdr *read_tcp = (struct tcphdr*)buf;
	printf("\nTCP:\n");
	printf("Destination port: %d\n",read_tcp -> th_dport);
	printf("Source port: %d\n",read_tcp ->th_sport);
	printf("Sequence number: %u\n",read_tcp ->th_seq);
	printf("Acknowledgement number: %u\n",read_tcp ->th_ack);
	printf("Data offset: %d\n",read_tcp->th_off);
	printf("Checksum: %d\n",read_tcp -> th_sum);

	buf += (4 * read_tcp->th_off);
	data_size -= (4 * read_tcp->th_off);
	
	printf("Destination port protocol:\n");
	tcp_udp_ports(buf, data_size, read_tcp->th_dport);	
	printf("Source port protocol:\n");
	tcp_udp_ports(buf, data_size, read_tcp->th_sport);
}

void proces_udp(unsigned char *buf,int data_size)
{
	struct udphdr *read_udp = (struct udphdr*)buf;
	printf("\nUDP:\n");
	printf("Destination port: %d\n",read_udp -> uh_dport);	
	printf("Source port: %d\n",read_udp -> uh_sport);
	printf("UDP length: %d\n",read_udp -> uh_ulen);
	printf("UDP checksum: %d\n",read_udp -> uh_sum);

	buf += (4 * read_udp -> uh_ulen);
	data_size -= (4 * read_udp -> uh_ulen);
	
	printf("Destination port protocol:\n");
	tcp_udp_ports(buf, data_size, read_udp->uh_dport);	
	printf("Source port protocol:\n");
	tcp_udp_ports(buf, data_size, read_udp->uh_sport);
}

void tcp_udp_ports(unsigned char *buf,int data_size,unsigned short port)
{
	switch(port)
	{
		case 7:
			proces_echo(buf,data_size);
		case 20:
			proces_ftp(buf,data_size);
		case 80:
			proces_http(buf,data_size);
		case 546:
		case 547:
			proces_dhcp(buf,data_size);
		default:
			printf("Port protocol not supported\n");
	}
}

void proces_echo(unsigned char *buf,int data_size)
{
	printf("\nEcho:\n");
	print_data(buf,data_size);
}

void proces_ftp(unsigned char *buf,int data_size)
{
	printf("\nFTP:\n");
	print_data(buf,data_size);
}

void proces_http(unsigned char *buf,int data_size)
{
	printf("\nHTTP:\n");
	print_data(buf,data_size);
}

void proces_dhcp(unsigned char *buf,int data_size)
{
	struct dhcpmsg *read_dhcp = (struct dhcpmsg*)buf;
	int i;

	printf("\nDHCP msg:\n");
	printf("OP: %d\n",read_dhcp -> op);
	printf("Htype: %d\n",read_dhcp -> htype);
	printf("Hlen: %d\n",read_dhcp -> hlen);
	printf("Hops: %d\n",read_dhcp -> hops);
	printf("XID: %d\n",read_dhcp -> xid);
	printf("Client IP address: %d\n",read_dhcp -> ciaddr);
	printf("Your IP address: %d\n",read_dhcp -> yiaddr);
	printf("Server IP address: %d\n",read_dhcp -> siaddr);
	printf("Getaway IP address: %d\n",read_dhcp -> giaddr);
	for(i = 1; i <= 3;i++)
	{
		printf("Client hardware address: %#02x:%#02x:%#02x:%#02x:%#02x:%#x\n",
				read_dhcp -> chaddr[i*0],
				read_dhcp -> chaddr[i*1],
				read_dhcp -> chaddr[i*2],	
				read_dhcp -> chaddr[i*3],
				read_dhcp -> chaddr[i*4],
				read_dhcp -> chaddr[i*5]
				);
	}
}

void proces_xtp(unsigned char *buf,int data_size)
{
	printf("\nXTP:\n");
}

void proces_snp(unsigned char *buf,int data_size)
{
	printf("\nSNP:\n");
}

void print_data(unsigned char *buf,int data_size)
{
	int i;
	for(i = 0; i < data_size; i++)
	{
		printf("%c",*(buf+i));
		if(i % 10 == 0)
			printf("\n");
	}
}

int main(int argc,char **argv)
{

	int sd,data_size,pid,child_status;	
	struct ifreq ifr ;
	unsigned char *buf = (unsigned char *)malloc(65536);	

	if(argc != 2)
	{	
		printf("Usage: %s interface\n",argv[0]);
		exit(0);
	}

	/* creating file descriptor from socket */	
	if( (sd = socket( AF_PACKET, SOCK_RAW, htons(ETH_P_ALL) ) ) == -1)
		exit_with_perror("socket\n");


	/* entering promisc mode */ 
	strncpy((char *)ifr.ifr_name, argv[1], IF_NAMESIZE);
	ifr.ifr_flags |= IFF_PROMISC;
	if( ioctl(sd, SIOCSIFFLAGS, &ifr ) != 0 )
	{
		exit_with_perror("ioctl\n");
	}

	if( ( pid = fork()) == 0 )
	{
		/* listening */
		for(;;)
		{	
			if( (data_size = recv(sd,buf,65536,0)) == -1 )	
				exit_with_perror("recv\n");
		
			proces_packet(buf,data_size);
		}
	}
	else if(pid > 0)
	{	
		getchar();
		kill(pid,9);	
		wait(&child_status);
		
		/* disabling promisc mode */
		strncpy((char *)ifr.ifr_name, argv[1], IF_NAMESIZE);
		ifr.ifr_flags &= ~IFF_PROMISC;
		if( ioctl(sd, SIOCSIFFLAGS, &ifr ) != 0 )
		{
			exit_with_perror("ioctl\n");
		}
					
		exit(0);
	
	}

	else
	{
		exit_with_perror("Fork\n");
	}

	close(sd);
	return 0;
}

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>


int exit_with_perror(char *msg) 
{
    perror(msg);
    exit(0);
}

void proces_packet(unsigned char *buf,int size)
{
	struct ip *read_ip = (struct ip*)buf;	
	switch(read_ip -> ip_p)
	{
		case 0:
			printf("HOPOPT\n");
			break;
		case 6:
			printf("TCP\n");
		      	break;

		case 17:
			printf("UDP\n");
		      	break;

		case 36:
			printf("XTP\n");
		      	break;

		case 109:
			printf("SNP\n");
		      	break;
		
		default:
			printf("%d\n",read_ip -> ip_p);
			break;
	}
}

int main(void)
{

	int sd,fd,data_size;	
	unsigned char *buf = (unsigned char *)malloc(65536);	
	
	/* opening file to save details of transition */
	if( (fd = open("details.txt",O_CREAT | O_RDONLY)) == -1 )
		exit_with_perror("open\n");
	
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
	close(fd);
	return 0;
}

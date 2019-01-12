#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#define TIMEOUT 5
#define BACKLOG 10

int close_flag;
typedef struct sockaddr sockaddr;
void sigchld_handler(int s) {
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
void exit_with_perror(char *msg)
{
	printf("%s\n",msg);
	exit(0);
}

int connect_to_server(char *address,char *port) 
{
    int sockfd;
    int gai_error;
    struct addrinfo hints, *servinfo, *p;

    /* here addrinfo is set to parameters chosen in options or default */
    
    /* zeroing hints structure */
    memset(&hints, 0, sizeof hints);
    
    hints.ai_family = AF_UNSPEC;
    
    hints.ai_socktype = SOCK_STREAM;
    
    hints.ai_flags = AI_PASSIVE;

    /* here the program iterates on all addreses of our machine and tryies to bind(),
     *  if it is succesful then it stops iterating and moves on */

    /* here it saves all addres to serverinfo */
    if ((gai_error = getaddrinfo(address, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai_error));
        exit(0);
    }

    /* here it iterates on returned addres */
    for (p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) 
    {
        printf("Could not connect\n");
        exit(0);
    }
    freeaddrinfo(servinfo);
    return sockfd;
}

int setup_server(char *port) {
    int sockfd, sockopt, rv, yes;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
            exit_with_perror("socket");

        sockopt =
            setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (sockopt == -1)
            exit_with_perror("setsockopt");

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)
        exit_with_perror("server: failed to bind");

    freeaddrinfo(servinfo);

    if (listen(sockfd, BACKLOG) == -1)
        exit_with_perror("listen");

    return sockfd;
}

void pass_data(int fd,int hostfd)
{
	int n;
	char data[1024];
	do{
	if ((n = recv(fd, data, sizeof(data), 0)) == -1)
       		perror("recv");

   	if (write(hostfd, data, n) == -1)
        	perror("write");
	}while(n != 0 );
	close_flag = 1;
}

int main(int argc, char **argv)
{
	int i,ret,number_of_descryptors,new_socket;
	char *host,*host_port,*port;
	int hostfd,*listenfd;
	struct sockaddr_in address; 	
    	int addrlen = sizeof(address);

	if( (argc - 1) % 3)
	        exit_with_perror("Wrong number of arguments");
	else
		number_of_descryptors = ( ( (argc - 1) / 3));

	close_flag = 0;

	struct pollfd *pfd; 
	pfd = calloc(number_of_descryptors,sizeof(struct pollfd));
	listenfd = calloc(number_of_descryptors,sizeof(int));

	for(i = 0 ; i < number_of_descryptors ; i++)
	{
		port = argv[3*i + 1]; 	
		if( (pfd[i].fd = setup_server(port)) < 0)
			exit_with_perror("SOCKET");
		pfd[i].events = POLLIN;

		listenfd[i] = pfd[i].fd;
	}

	for(;;)
	{
		printf("\n\nBEFORE POLL()\n");
		for(i = 0 ; i < number_of_descryptors ; i++)
			printf("Poll descryptors: %d\n",pfd[i].fd);
		if( (ret = poll(pfd,number_of_descryptors,TIMEOUT * 60 * 1000)) < 0)
			exit_with_perror("POLL");
		else if (ret == 0)
			exit_with_perror("TIMEOUT");

		for(i = 0 ; i < number_of_descryptors ; i++)
		{

 			host = argv[ (i*3 + 2) ];
			host_port= argv[ (i*3 + 3) ];
      		
			if(pfd[i].revents == 0)
        			continue;
			if(pfd[i].revents & POLLIN)
			{
				if(pfd[i].fd == listenfd[i])
				{
					new_socket = accept(pfd[i].fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
					if (new_socket < 0) 
					{
						printf("%d\n",errno);
						if( errno != EWOULDBLOCK)
  							exit_with_perror("accept"); 
						break;
					}
					pfd[i].fd = new_socket;
					printf("AFTER ACCEPT: %d - %d\n",pfd[i].fd,listenfd[i]);
				}
				else
				{
					printf("HOST: %s, PORT: %s\n", host,host_port);
					hostfd = connect_to_server(host,host_port);
					pass_data(pfd[i].fd,hostfd);		
					if(close_flag)
					{
						//close(pfd[i].fd);
						close(hostfd);
						pfd[i].fd = listenfd[i];
						close_flag = 0;
					}
				}
			}
		}
	}

  	for (i = 0; i < number_of_descryptors; i++)
 	{
    		if(pfd[i].fd >= 0)
      			close(pfd[i].fd);
  	}

	free(pfd);
	free(listenfd);
	return 0;
}

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BACKLOG 10
#define MAXDATASIZE 512
#define MAXLINE 1024

typedef struct sockaddr sockaddr;
int flag_server,flag_udp,flag_ip4,flag_ip6;

void zero_flags();
void exit_with_perror(char *msg);
void sigchld_handler(int s); 
void *get_in_addr(sockaddr *sa); 
int connect_to_server(char *address,char *port); 
void send_data(int socketfd);
int setup_server(char *port); 

void zero_flags()
{
	flag_server = 0;
	flag_udp = 0;
	flag_ip4 = 0;
	flag_ip6 = 0;
}

void exit_with_perror(char *msg)
{
	printf("%s\n",msg);
	exit(0);
}

void sigchld_handler(int s) 
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void *get_in_addr(sockaddr *sa) 
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in *)sa)->sin_addr);
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int connect_to_server(char *address,char *port) 
{
    int sockfd;    // deskryptor socket
    int gai_error; // kod bledu dla gai
    char server_ip[INET6_ADDRSTRLEN];
    void *inaddr;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    
    if(flag_ip4)
	hints.ai_family = AF_INET;
    else if(flag_ip6)
	hints.ai_family = AF_INET6;
    else
    	hints.ai_family = AF_UNSPEC;
    
    if(flag_udp)
	hints.ai_socktype = SOCK_DGRAM;
    else 
    	hints.ai_socktype = SOCK_STREAM;
    
    hints.ai_flags = AI_PASSIVE;

    if ((gai_error = getaddrinfo(address, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai_error));
        exit(0);
    }

    // iteracja po zwróconych adresach,
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

        // przerwij po znalezieniu pierwszego poprawnego adresu
        break;
    }

    // nie udalo sie polaczyc
    if (p == NULL) {
        printf("Nie udało się połączyć\n");
        exit(0);
    }

    // translacja adresu z postaci sieciowej do formatu prezentacji
    inaddr = get_in_addr(p->ai_addr);
    inet_ntop(p->ai_family, inaddr, server_ip, sizeof(server_ip));
    printf("client: connecting to %s\n", server_ip);

    freeaddrinfo(servinfo);
    return sockfd;
}

void send_data(int socketfd)
{
	char data[MAXLINE];

	for(;;)
	{
		if( fgets(data,MAXLINE,stdin) != NULL)
		{
			write(socketfd,data,strlen(data));
		}
	}
}

int setup_server(char *port) 
{
    int sockfd, sockopt, rv, yes;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    
    if(flag_ip4)
	hints.ai_family = AF_INET;
    else if(flag_ip6)
	hints.ai_family = AF_INET6;
    else
    	hints.ai_family = AF_UNSPEC;
    
    if(flag_udp)
	hints.ai_socktype = SOCK_DGRAM;
    else 
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

void write_read_data(int socketfd)
{
    int n;
    char data[1024];
    for(;;)
    {
   	 if ((n = recv(socketfd, data, sizeof(data), 0)) == -1)
       		 perror("recv");

   	 if (write(1, data, n) == -1)
        	perror("write");
    }
}

void run_server(int socketfd)
{

    	    int pid,new_fd;
    	    char presentation_addr[INET6_ADDRSTRLEN];
   	    struct sockaddr_storage their_addr;
	    socklen_t sin_size;
	    struct sigaction sa;
	
    	    sa.sa_handler = sigchld_handler;
	    sigemptyset(&sa.sa_mask);
	    sa.sa_flags = SA_RESTART;

	    if (sigaction(SIGCHLD, &sa, NULL) == -1)
	    	exit_with_perror("sigaction");

	    printf("server: waiting for connections...\n");

	    while (1) 
	    {
		sin_size = sizeof their_addr;
		if( (new_fd = accept(socketfd, (struct sockaddr *)&their_addr, &sin_size) ) == -1)
			exit_with_perror("accept");

		inet_ntop(their_addr.ss_family,
		get_in_addr((struct sockaddr *)&their_addr),
		presentation_addr, sizeof(presentation_addr));
		printf("server: got connection from %s\n", presentation_addr);

		if ((pid = fork()) == 0) 
		{
		    close(socketfd);
		    write_read_data(new_fd);
		    exit(0);
		} 
		else if (pid > 0) 
		{
		    close(new_fd);
		}
	       	else 
		{
		    exit_with_perror("fork");
		}
	    }
		
}

int main(int argc, char **argv)
{
	int opt,socketfd;

	zero_flags();
	while ((opt = getopt(argc, argv, "lu46")) != -1)
       	{
               switch (opt)
	       {
               		case 'l':
   				flag_server = 1;
                	   	break;
               		case 'u':
				flag_udp = 1;
   	        	        break;
			case '4':
				flag_ip4 = 1;
				break;
			case '6':
				flag_ip6 = 1;
				break;
			default:
				if( argc != 3)
				{	
					printf("Usage: %s host port [-l] [-u] [-4] [-6]\n",argv[0]);
					exit(0);
				}
               }
	}

	if(flag_server)
	{

	    socketfd = setup_server(argv[optind]);
	    run_server(socketfd);
	}
	else	
	{
		socketfd = connect_to_server(argv[optind],argv[optind + 1]);
		send_data(socketfd);
		close(socketfd);
	}
	return 0;
}

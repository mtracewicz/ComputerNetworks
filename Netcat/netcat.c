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

/* for lisen() */
#define BACKLOG 10

/* it is a maximal lenght of data which can be send or recived by the program */
#define MAXLINE 1024

/* type definition for sockaddr structure */
typedef struct sockaddr sockaddr;

/* flags for options, in order: -l,-u,-4-6 */
int flag_server,flag_udp,flag_ip4,flag_ip6;

void zero_flags();
void exit_with_perror(char *msg);
void sigchld_handler(int s); 
void *get_in_addr(sockaddr *sa); 
int connect_to_server(char *address,char *port); 
void send_data(int socketfd);
int setup_server(char *port); 
void write_read_data(int socketfd);
void run_server(int socketfd);

/* this function zeros all of the flags which can be used in this program */
void zero_flags()
{
	flag_server = 0;
	flag_udp = 0;
	flag_ip4 = 0;
	flag_ip6 = 0;
}

/* this function allows to write a readable error message and exit program */
void exit_with_perror(char *msg)
{
	printf("%s\n",msg);
	exit(0);
}

/* handler for SIGCHLD signal to allow non blocking use of fork() and awoid creating zombies */
void sigchld_handler(int s) 
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* this function returns IPv4 or IPv6 addres depending on which protocol is used */
void *get_in_addr(sockaddr *sa) 
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in *)sa)->sin_addr);
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

/* this function connects our client to provided server and port */
int connect_to_server(char *address,char *port) 
{
    int sockfd;
    int gai_error;
    int flags;
    char server_ip[INET6_ADDRSTRLEN];
    void *inaddr;
    struct addrinfo hints, *servinfo, *p;

    /* here addrinfo is set to parameters chosen in options or default */
    
    /* zeroing hints structure */
    memset(&hints, 0, sizeof hints);
    
    /* if IPv4 flag is set we chose server to operate on it, if IPv6 is set we chose server to operate on in,
     * if non is set then we chose unspecified addres family */
    if(flag_ip4)
	hints.ai_family = AF_INET;
    else if(flag_ip6)
	hints.ai_family = AF_INET6;
    else
    	hints.ai_family = AF_UNSPEC;
    
    /* if UDP flag is set we chose our server to operate on it, if it's not then we chose TCP as default */ 
    if(flag_udp)
	hints.ai_socktype = SOCK_DGRAM;
    else 
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

    flags = fcntl(sockfd,F_GETFL);
    fcntl(sockfd,F_SETFL,flags | O_NONBLOCK);

    /* translation of addres into presentation format */
    inaddr = get_in_addr(p->ai_addr);
    inet_ntop(p->ai_family, inaddr, server_ip, sizeof(server_ip));
    printf("client: connecting to %s\n", server_ip);

    freeaddrinfo(servinfo);
    return sockfd;
}

/* this function sends data read from STDIN and sends it to host and 
 * if there is a response it's writen on STDOUT */
void send_data(int socketfd)
{
	char data[MAXLINE];
	int numbytes;
	for(;;)
	{
		/* reading data from STDIN and sending it to host */
		if( fgets(data,MAXLINE,stdin) != NULL)
		{
			write(socketfd,data,strlen(data));
		}

		/* if there is a response then it's read and writen here */
		/* Known bug ! after sending a request which is expected to return an anwser
		 * user needs to send one more message in order for anwser to be writen on STDOUT */ 
    		while ((numbytes = read(socketfd, data, MAXLINE)) > 0)
        		write(1, data, numbytes);

   		if (numbytes == -1 && errno == 11)
		{
			continue;
		}
		else if( numbytes < 0)
			exit_with_perror("read");
	}
}

/* function which sets up our server (both UDP and TCP) if server flag is set */
int setup_server(char *port) 
{
    int sockfd, sockopt, rv, yes;
    struct addrinfo hints, *servinfo, *p;
    
    /* here addrinfo is set to parameters chosen in options or default */
    
    /* zeroing hints structure */
    memset(&hints, 0, sizeof(hints));
    
    /* if IPv4 flag is set we chose server to operate on it, if IPv6 is set we chose server to operate on in,
     * if non is set then we chose unspecified addres family */
    if(flag_ip4)
	hints.ai_family = AF_INET;
    else if(flag_ip6)
	hints.ai_family = AF_INET6;
    else
    	hints.ai_family = AF_UNSPEC;
    
    /* if UDP flag is set we chose our server to operate on it, if it's not then we chose TCP as default */ 
    if(flag_udp)
	hints.ai_socktype = SOCK_DGRAM;
    else 
    	hints.ai_socktype = SOCK_STREAM;
    
    hints.ai_flags = AI_PASSIVE;

    /* here the program iterates on all addreses of our machine and tryies to bind(),
     *  if it is succesful then it stops iterating and moves on */

    /* here it saves all addres to serverinfo */
    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    
    /* here it iterates on returned addres */
    for (p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
            exit_with_perror("socket");

        sockopt = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
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

    /* we need to use listen() only if we use TCP */
    if(!flag_udp)
    {
	    if (listen(sockfd, BACKLOG) == -1)
        	exit_with_perror("listen");
    }
    return sockfd;
}

/* this function handles reading and anwsering request */
void write_read_data(int socketfd)
{
    int n;
    char data[1024];
    struct sockaddr udp_addr;
    socklen_t udp_len;
    udp_len = sizeof(udp_addr);
    
    /* both parts of if statment read data and write it on STDOUT */ 

    /* if UDP flag is set we handle it once and then exit function */
    if(flag_udp)
    {

		 if ((n = recvfrom(socketfd, data, sizeof(data),0,&udp_addr,&udp_len)) == -1)
       			 perror("recvfrom");
		
   		 if (write(1, data, n) == -1)
        		perror("write");
    }
    /* if it's not then we enter an infinite loop which is necessary to handle TCP requests */
    else
    {
    	for(;;)
    	{ 

		 if ((n = recv(socketfd, data, sizeof(data), 0)) == -1)
       			 perror("recv");

   		 if (write(1, data, n) == -1)
        		perror("write");
   	}
    }
}

/* this function is where server fork is done */
void run_server(int socketfd)
{

    	    int pid,new_fd;
	    socklen_t sin_size;
	    struct sigaction sa;
	    struct sockaddr_storage their_addr;
   	    char presentation_addr[INET6_ADDRSTRLEN];

	    /* we register a signal handler to wait() for compleated child process in TCP server */ 
	    sa.sa_handler = sigchld_handler;
	    sigemptyset(&sa.sa_mask);
	    sa.sa_flags = SA_RESTART;
		
	    if (sigaction(SIGCHLD, &sa, NULL) == -1)
	    	exit_with_perror("sigaction");

	    printf("server: waiting for connections...\n");

	    while (1) 
	    {
		sin_size = sizeof (their_addr);
		
		/* if UDP flag is set the program goes straight into handeling send request */
		if(flag_udp)
			write_read_data(socketfd);
		/*for TCP server  we make it concurrent using fork */
		else if(!flag_udp)
		{
			/* awaiting client */
			if( (new_fd = accept(socketfd, (struct sockaddr *)&their_addr, &sin_size) ) == -1)
				exit_with_perror("accept");
			
    			/* translation of addres into presentation format */
		        inet_ntop(their_addr.ss_family,  get_in_addr((struct sockaddr *)&their_addr),presentation_addr, sizeof(presentation_addr));
        		printf("server: got connection from %s\n", presentation_addr);

			/* child handles reading and anwsering requests	*/
			if ((pid = fork()) == 0) 
			{
		    	    close(socketfd);
			    write_read_data(new_fd); 
		    	    exit(0);
			} 
			/* parent closes descryptor and awaits next client */
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
		
}

int main(int argc, char **argv)
{
	/* veriable for switchopt and file descryptor for socket */
	int opt,socketfd;

	/* zeroing flags which will get set by optinons in argv */
	zero_flags();
	
	/* loop for seting flags */
	while ((opt = getopt(argc, argv, "lu46")) != -1)
       	{
               switch (opt)
	       {
		        /* indicates that program will work in server mode */
               		case 'l':
   				flag_server = 1;
                	   	break;
			/* indicates that program will use UDP */
               		case 'u':
				flag_udp = 1;
   	        	        break;
			/* indicates that program will use IPv4 */
			case '4':
				flag_ip4 = 1;
				break;
			/* indicates that program will use IPv6 */
			case '6':
				flag_ip6 = 1;
				break;
			/* default case checks if there is correct amount of arguments for default program call */
			default:
				if( argc != 3)
				{	
					printf("Usage: %s host port [-l] [-u] [-4] [-6]\n",argv[0]);
					exit(0);
				}
               }
	}

	/* if server flag is set the program sets up server on provided port and then runs it */
	if(flag_server)
	{

		socketfd = setup_server(argv[optind]);
		run_server(socketfd);
	}
	/* if server flag isn't set the program connects to host on port which were provided on program call */
	else	
	{
		socketfd = connect_to_server(argv[optind],argv[optind + 1]);
		send_data(socketfd);
		close(socketfd);
	}
	return 0;
}

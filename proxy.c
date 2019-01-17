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

int close_flag_read, close_flag_write;
typedef struct sockaddr sockaddr;

void sigchld_handler(int s)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void exit_with_perror(char *msg)
{
	printf("%s\n", msg);
	exit(0);
}

int connect_to_server(char *address, char *port)
{
	int sockfd;
	int gai_error;
	struct addrinfo hints, *servinfo, *p;

	/* here addrinfo is set to parameters chosen in options or default */

	/* zeroing hints structure */
	memset(&hints, 0, sizeof( hints ));

	hints.ai_family = AF_UNSPEC;

	hints.ai_socktype = SOCK_STREAM;

	hints.ai_flags = AI_PASSIVE;

	/* here the program iterates on all addreses of our machine and tryies to bind(),
     *  if it is succesful then it stops iterating and moves on */

	/* here it saves all addres to serverinfo */
	if ((gai_error = getaddrinfo(address, port, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai_error));
		exit(0);
	}

	/* here it iterates on returned addres */
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd == -1)
		{
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		printf("Could not connect to: %s %s\n",address,port);
		return -1;
	}

	freeaddrinfo(servinfo);
	return sockfd;
}

int setup_server(char *port)
{
	int sockfd, sockopt, rv, yes;
	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd == -1)
			exit_with_perror("socket");

		sockopt = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		if (sockopt == -1)
			exit_with_perror("setsockopt");

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
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

void pass_data(int in_fd, int out_fd)
{
	int n;
	char data[1024];
	printf("IN:%d,OUT:%d\n",in_fd,out_fd);
	do
	{
		if ( (n = recv(in_fd, data, sizeof(data), 0)) == -1)
		{
			if (errno != EAGAIN || errno != EWOULDBLOCK)
			{
				perror("recv");	
			}
			printf("n=%d\n",n);
			break;
		}
		else if( n == 0)
		{
			printf("n=%d\n",n);
			close_flag_read = 1;
			break;
		}

		if (write(out_fd, data, n) == -1)
		{
			if (errno != EAGAIN || errno != EWOULDBLOCK)
			{
				perror("write");
				close_flag_write = 1;
			}
			break;
		}

	} while (1);
}

int main(int argc, char **argv)
{
	int i, ret, number_of_descryptors, new_socket, flags;
	char *host, *host_port, *port;
	int *listenfd;
	struct sockaddr_in address;
	int addrlen = sizeof(address);

	if ((argc - 1) % 3)
		exit_with_perror("Wrong number of arguments");
	else
		number_of_descryptors = (((argc - 1) / 3));

	close_flag_read = 0;
	close_flag_write = 0;

	struct pollfd *pfd;
	pfd = calloc(2 * number_of_descryptors, sizeof(struct pollfd));
	listenfd = calloc(number_of_descryptors, sizeof(int));

	for (i = 0; i < number_of_descryptors; i++)
	{
		/*reading local port, host and host port from arguments*/
		port = argv[3 * i + 1];
		host = argv[(i * 3 + 2)];
		host_port = argv[(i * 3 + 3)];

		/* setting up servers on port and putting it in struct pollfd */
		if ((pfd[i].fd = setup_server(port)) < 0)
			exit_with_perror("SOCKET");
		pfd[i].events = POLLIN;
		listenfd[i] = pfd[i].fd;
		printf("LOCAL_PORT_LISTEN:%s,FD:%d\n",port,pfd[i].fd);
		/* connecting to host on host_port and putting it in struct pollfd, then wetting it to be nonblocking */
		pfd[i + number_of_descryptors].fd = connect_to_server(host, host_port);
		pfd[i + number_of_descryptors].events = POLLIN;
		flags = fcntl(pfd[i + number_of_descryptors].fd, F_GETFL);
		fcntl(pfd[i + number_of_descryptors].fd, F_SETFL, flags | O_NONBLOCK);
		printf("HOST:%s,PORT:%s,FD:%d\n",host,host_port,pfd[i + number_of_descryptors].fd);
	}

	for (;;)
	{
		if ((ret = poll(pfd, 2 * number_of_descryptors, TIMEOUT * 60 * 1000)) < 0)
			exit_with_perror("POLL");
		else if (ret == 0)
			exit_with_perror("TIMEOUT");

		for (i = 0; i < 2 * number_of_descryptors; i++)
		{
			if (pfd[i].revents == 0)
				continue;
			if ((pfd[i].revents & POLLIN) == POLLIN)
			{
				if (i < number_of_descryptors && pfd[i].fd == listenfd[i])
				{

					if ((new_socket = accept(pfd[i].fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
					{
						if (errno != EWOULDBLOCK)
							exit_with_perror("accept");
						break;
					}

					pfd[i].fd = new_socket;
					flags = fcntl(pfd[i].fd, F_GETFL);
					fcntl(pfd[i].fd, F_SETFL, flags | O_NONBLOCK);
					printf("LOCAL_PORT_ACCEPT:%s,FD:%d\n",port,pfd[i].fd);
				}
				else
				{
					if (i < number_of_descryptors)
					{
						pass_data(pfd[i].fd, pfd[i + number_of_descryptors].fd);
						if(close_flag_read)
						{
							close(pfd[i].fd);
							pfd[i].fd = listenfd[i];
							close_flag_read = 0;
						}
					}
					else
					{
						pass_data(pfd[i].fd, pfd[i - number_of_descryptors].fd);
						if(close_flag_read)
						{
							host = argv[((i-number_of_descryptors) * 3 + 2)];
							host_port = argv[((i-number_of_descryptors) * 3 + 3)];
							if( (pfd[i].fd = connect_to_server(host, host_port)) != -1)
							{
								pfd[i].events = POLLIN;
								flags = fcntl(pfd[i].fd, F_GETFL);
								fcntl(pfd[i].fd, F_SETFL, flags | O_NONBLOCK);
							}
							close_flag_read = 0;
						}
					}
	
				}
			}
		}
	}

	for (i = 0; i < (2 * number_of_descryptors); i++)
	{
		if (pfd[i].fd >= 0)
			close(pfd[i].fd);
	}

	free(pfd);
	free(listenfd);
	return 0;
}

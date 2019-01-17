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

/* flag indicating when to close connection */ 
int close_flag;
typedef struct sockaddr sockaddr;

/* function returning addres for correct IP version */
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

/* function which writes msg on STDERR and exits */
void exit_with_perror(char *msg)
{
	perror(msg);
	exit(0);
}

/* function connecting program to server on provided address and port */
int connect_to_server(char *address, char *port)
{
	int sockfd;
	int gai_error;
	struct addrinfo hints, *servinfo, *p;

	/* here addrinfo is set to parameters chosen in options or default */

	/* zeroing hints structure */
	memset(&hints, 0, sizeof(hints));

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

		/* here it tryies to connect */
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("client: connect");
			continue;
		}
		
		/* breaks on first succesfull connect */
		break;
	}

	/* if the program didn't connect it writes it and returns -1 */
	if (p == NULL)
	{
		printf("Could not connect to: %s %s\n", address, port);
		return -1;
	}

	freeaddrinfo(servinfo);
	return sockfd;
}

/* function to setup server on given port */
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

/* this function reads data from in_fd and writes it to out_fd */
void pass_data(int in_fd, int out_fd)
{
	int n;
	char data[1024];

	/* infinite loop to process data */
	do
	{
		if ((n = recv(in_fd, data, sizeof(data), 0)) == -1)
		{
			/* we break if there is no more data to read or an error ocured */
			if (errno != EAGAIN || errno != EWOULDBLOCK)
			{
				perror("recv");
			}
			break;
		}
		else if (n == 0)
		{
			/* if the connection got ended we set the close_flag and break */
			close_flag = 1;
			break;
		}

		/* we write as long as there is smothing to write */
		if (write(out_fd, data, n) == -1)
		{
			/* if an error ocures we break */
			if (errno != EAGAIN || errno != EWOULDBLOCK)
			{
				perror("write");
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

	/* we're checking if the number of arguments is right and if it is we
	set number_of_descryptors to the number of complete set of arguments */
	if ((argc - 1) % 3)
		exit_with_perror("Wrong number of arguments");
	else
		number_of_descryptors = (((argc - 1) / 3));

	close_flag = 0;

	/* we allocate(and by using calloc zero) memory for struct pollfd* and int* to register
	all needed descryptors pfd gets two times the space becouse it allso stores
	client descriptors and listenfd does not*/ 
	struct pollfd *pfd;
	pfd = calloc(2 * number_of_descryptors, sizeof(struct pollfd));
	listenfd = calloc(number_of_descryptors, sizeof(int));

	/* here we register all of provided server and clients */
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

		/* connecting to host on host_port and putting it in struct pollfd,
		then setting it to be nonblocking */
		pfd[i + number_of_descryptors].fd = connect_to_server(host, host_port);
		pfd[i + number_of_descryptors].events = POLLIN;
		flags = fcntl(pfd[i + number_of_descryptors].fd, F_GETFL);
		fcntl(pfd[i + number_of_descryptors].fd, F_SETFL, flags | O_NONBLOCK);
	}

	/* this is the main loop of our program in which we use poll() to perrform our
	comunication */
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
					printf("LOCAL_PORT_ACCEPT:%s,FD:%d\n", port, pfd[i].fd);
				}
				else
				{
					if (i < number_of_descryptors)
					{
						pass_data(pfd[i].fd, pfd[i + number_of_descryptors].fd);
						if (close_flag)
						{
							close(pfd[i].fd);
							pfd[i].fd = listenfd[i];
							close_flag = 0;
						}
					}
					else
					{
						pass_data(pfd[i].fd, pfd[i - number_of_descryptors].fd);
						if (close_flag)
						{
							host = argv[((i - number_of_descryptors) * 3 + 2)];
							host_port = argv[((i - number_of_descryptors) * 3 + 3)];
							if ((pfd[i].fd = connect_to_server(host, host_port)) != -1)
							{
								pfd[i].events = POLLIN;
								flags = fcntl(pfd[i].fd, F_GETFL);
								fcntl(pfd[i].fd, F_SETFL, flags | O_NONBLOCK);
							}
							close_flag = 0;
						}
					}
				}
			}
		}
	}

	/* closing all descryptors before exit */
	for (i = 0; i < (2 * number_of_descryptors); i++)
		close(pfd[i].fd);
	/* freeing memory allocated for struct pollfd *pfd and int *listenfd */ 
	free(pfd);
	free(listenfd);
	return 0;
}

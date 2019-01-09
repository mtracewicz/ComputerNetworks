#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h> 
#include <unistd.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#define TIMEOUT 5

void exit_with_perror(char *msg)
{
	printf("%s\n",msg);
	exit(0);
}


struct pollfd* set_poll(int number_of_descryptors)
{

	struct pollfd pfd[number_of_descryptors];
	int i;
	for(i = 0 ; i < number_of_descryptors ; i++)
	{
		if( (pfd[i].fd = socket(AF_INET,SOCK_STREAM:TCP,0)) < 0)
			exit_with_perror("SOCKET");
		
		pfd[i].events = POLLIN;
	}

	return pfd;
}

void pass_data(int fd,int arc,char **argv);

int main(int argc, char **argv)
{
	int i,ret,number_of_descryptors,new_socket,local_port,host,host_port;
	struct sockaddr_in address; 	
    	int addrlen = sizeof(address);
	if( (argc - 1) % 3)
	        exit_with_perror("Wrong number of arguments");
	else
		number_of_descryptors = ( ( (argc - 1) / 3));

	struct pollfd pfd[number_of_descryptors] = set_poll(number_of_descryptors);
	
	for(;;)
	{
		if( (ret = poll(pfd,number_of_descryptors,TIMEOUT * 1000)) < 0)
			exit_with_perror("POLL");
		else if (ret == 0)
			exit_with_perror("TIMEOUT");


		for(i = 0 ; i < number_of_descryptors ; i++)
		{
			if(pfd[i].revents & POLLIN)
			{
				local_port =  htons( strtol((i*3 + 1),NULL,10) );
 			 	host =  htons( strtol((i*3 + 2),NULL,10 ));
				host_port=  htons( strtol((i*3 + 3),NULL,10 ));

				address.sin_family = AF_INET; 
 				address.sin_addr.s_addr = INADDR_ANY; 
  				address.sin_port = local_port;
    				if(bind(pfd[i].fd, (struct sockaddr *)&address, sizeof(address)) < 0) 
   					exit_with_perror("bind");
				
				if (listen(pfd[i].fd, 3) < 0) 
    					exit_with_perror("listen"); 

    				if ((new_socket = accept(pfd[i].fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) 
  					exit_with_perror("accept"); 


			}			
		}
	}

  	for (i = 0; i < number_of_descryptors; i++)
 	{
    		if(pfd[i].fd >= 0)
      			close(pfd[i].fd);
  	}
	return 0;
}

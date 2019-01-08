#include <stdio.h>
#include <errno.h>
#include <poll.h>
void exit_with_perror(char *msg)
{
	printf("%s\n",msg);
	exit(0);
}

int main(int argc, char **argv)
{
	int i,number_of_descryptors,fd,local_port,host_port;

	if( (argc - 1) % 3)
	       exit_with_perror("Wrong number of arguments");
	else
		number_of_descryptors = ( (argc - 1) / 3);

	struct pollfd pfd[number_of_descryptors];
	for(i = 0 ; i < number_of_desryptors ; i++)
	{
		fd = socket
	}
	for(;;)
	{

	}
	return 0;
}

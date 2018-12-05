#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAXLINE 1024
#define MAXNAME 25

struct my_msgbuf
{
	long mtype;	
	char usrname[MAXNAME];
	char mtext[MAXLINE];
};

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>

#define TOK "my_sem.h"

union semun 
{
	int val;
	struct semid_ds *buf;
	ushort *array;
}

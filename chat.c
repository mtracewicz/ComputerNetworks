#include "my_msq.h"
#include "my_sem.h"
#include "chat.h"

int main(int argc, char **argv)
{
	/* id veriables for message queue, semaphore, shared memory */
	int qid, sid, mid;
	/* keys variables used to create ids of veriables above */
	key_t qkey, skey, mkey;

	/* creating all needed IPC devices */

    	/* obtaining keys */
	
	if((qkey = ftok("my_msq.h", '1')) == -1) 
	{  
		perror("msq ftok");
	        exit(1);
	};

	if((skey = ftok("my_sem.h", '2')) == -1) 
	{  
		perror("sem ftok");
	        exit(1);
	};

	if((mkey = ftok("chat.h", '3')) == -1) 
	{  
		perror("mem ftok");
	        exit(1);
	};

	/* connecting or creating a message queue */
	if ((qid = msgget(qkey, 0644 | IPC_CREAT)) == -1) 
	{
		perror("msgget");
		exit(1);
	};

	/* creating semaphore */
	if ((sid = semget(skey, 1 |IPC_EXCL | IPC_CREAT) == -1) 
	{
		if ((sid = semget(skey, 1 | IPC_CREAT) == -1) 
		{
			perror("semget");
			exit(1);		
		};

		arg.val = 1;
    		if (semctl(sid, 0, SETVAL, arg) == -1) 
    		{
        		perror("semctl");
        		exit(1);
    		}	
	};


	/* creating shared memory */
	if ((shmid = shmget(mkey, SHM_SIZE, 0644 | IPC_CREAT)) == -1)
    	{
       	 	perror("shmget");
        	exit(1);
    	};

	/* creating all needed IPC devices */

    return 0;
}

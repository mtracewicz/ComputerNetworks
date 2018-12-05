#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>
#include "my_msq.h"
#include "my_sem.h"

int main(int argc, char **argv)
{
	/* id veriables for message queue, semaphore, shared memory */
	int qid, sid, mid;
	/* keys variables used to create ids of veriables above */
	key_t qkey, skey, mkey;
	/* struct for sending and reciving msg*/
	struct my_msgbuf buf;	
	struct my_msgbuf bufrcv;
	/* veriable used to register proces into chat */	
	pid_t procesid;
	/* array of registered users */
	int *registered;
	/* veriables used to register user */
	char username[25];
	uid_t userid;
	struct passwd *pwd;
	char newpid[5] = {0};
	/* iterators, controlers and tmp veriables */
	int i = 0,c = 0,tmp = 0;
	/* semaphore veriable*/
	union semun arg;

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
	if ((sid = semget(skey, 1 ,IPC_EXCL | IPC_CREAT) == -1)) 
	{
		if ((sid = semget(skey, 1 , IPC_CREAT) == -1)) 
		{
			perror("semget");
			exit(1);		
		}

		arg.val = 1;
    		if (semctl(sid, 0, SETVAL, arg) == -1) 
    		{
        		perror("semctl");
        		exit(1);
    		}	
	};


	/* creating shared memory */
	if ((mid = shmget(mkey,sizeof(int[15]), 0644 | IPC_EXCL | IPC_CREAT)) == -1)
    	{ 
		if ((mid = shmget(mkey,sizeof(int[15]), 0644 | IPC_CREAT)) == -1)
		{
			perror("shmget");
			exit(1);		
		}
	
		registered = (int*)shmat(mid,0,0);
		for(i = 0 ; i < 15;i++)
		{
			registered[i] = 0;
		}
    	}
	else
	{
		registered = (int*)shmat(mid,0,0);
	}

	/* created all needed IPC devices
	 * now we will register user */
	procesid = getpid();	
	for(i = 0 ; i < 15;i++)
	{
		if(registered[i] == 0 && c == 0)
		{
			registered[i] = procesid;
			c = 1;
		}
	}
	if( c == 0 )
	{
		printf("Chat is being used by maximum users right now, please try again later\n");
		exit(0);
	}
	/* creating username */
	userid = getuid();
	sprintf(newpid, "%d", (int)procesid);
	pwd = getpwuid(userid);
	strcat(username,pwd -> pw_name);
	strcat(username," ");
	strcat(username,newpid);
	strncpy(buf.usrname, username, 25);
	/* chat starts */
	for(;;)
	{
		    
		/* reciving msg */ 
		if (msgrcv(qid, (struct msgbuf *)&bufrcv, sizeof(buf), procesid, 0) == -1) 
		{
               		 perror("msgrcv");
           		 exit(1);
               	}
        	 printf("%s: %s\n", buf.usrname, buf.mtext);	
		 
		 /* sending msg*/
		 while( fgets(buf.mtext, MAXLINE, stdin) != NULL ) 
	  	 {
			if(buf.mtext == "/exit")
				exit(1);       	
			for( i = 0 ; i < 15 ; i++)    
			{
				if( registered[i] == 0)
					continue;
				else
				{ 
					buf.mtype = registered[i];	
					if (msgsnd(qid,(struct msgbuf *)&buf, sizeof(buf),0) == -1)
						perror("msgsnd");
				}
			}
	         } 
	}	
	/* unregistering */
	for( i = 0 ; i < 15 ; i++)    
	{
		if( registered[i] == procesid)
			registered[i] == 0;
	}
	/* detaching shered memory */
	shmdt((void *) registered);
    	return 0;
}

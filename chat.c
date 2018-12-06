#include <pwd.h>
#include <stdio.h>
#include <errno.h>
#include "my_msq.h"
#include "my_sem.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>

/* flags to control arguments input */
int flagmsq,flagshm,flagsem;

/* function to support ctrl+c */
void signalc(int s);

/* function which displays help if -h argument got used */
void help();

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
	uid_t userid;
	struct passwd *pwd;
	char newpid[6] = {0};
	
	/* iterators, controlers veriables */
	int i = 0,c = 0,ctr = 0;
	
	/* semaphore veriable*/
	union semun arg;
  	struct sembuf sb;

	/* veriable for forking */
	pid_t child;

	/* veriables for signal control */
    	sigset_t iset;
    	struct sigaction act;
	
	/* option variable */
	int opt;

	flagshm = 0;
	flagmsq = 0;
	flagsem = 0;
	if(argc > 1)
	{
		while ((opt = getopt(argc, argv, "m:q:s:h")) != -1) 
		{
        		switch (opt) 
			{
               			case 'm':
                   			flagshm = 1;
                   			mid = atoi(optarg);
					break;
               			case 'q':
                   			flagmsq = 1;
                   			qid = atoi(optarg);
					break;
               			case 's':
                   			flagsem = 1;
                   			sid = atoi(optarg);
                   			break;
				case 'h':
					help();
					exit(4);	
				default:
                   			fprintf(stderr, "Usage: %s [-m shmid] [-q msqid] [-s semid] [-h]\n",argv[0]);
                   			exit(3);
               		}
           	}
	}

	/* control of CTRL + C */
    	sigemptyset(&iset);
    	act.sa_handler = &signalc;
    	act.sa_mask = iset;
	act.sa_flags = 0;
    	sigaction(SIGINT, &act, NULL);

	if(flagmsq == 0)
	{
		/* creating all needed IPC devices */
			/* obtaining keys */
		if((qkey = ftok("my_msq.h", '1')) == -1) 
		{  
			perror("msq ftok");
			    exit(1);
		};
		/* connecting or creating a message queue */
		if ((qid = msgget(qkey, 0644 | IPC_CREAT)) == -1) 
		{
			perror("msgget");
			exit(1);
		};
	}
	if(flagmsq == 0)
	{
		if((skey = ftok("my_sem.h", '2')) == -1) 
		{  
			perror("sem ftok");
			    exit(1);
		};
		/* creating semaphore */
		if ( (sid = semget(skey, 1 ,0666 | IPC_EXCL | IPC_CREAT) ) == -1 ) 
		{	
			if ((sid = semget(skey, 1 , 0666 |IPC_CREAT)) == -1) 
			{
				perror("semget");
				exit(1);		
			}
		}
		else
		{	
			arg.val = 1;
				if (semctl(sid, 0, SETVAL, arg) == -1) 
				{
		   			perror("semctl");
		    		exit(1);
			}	
		}
	}
	if(flagmsq == 0)
	{
		if((mkey = ftok("chat.c", '3')) == -1) 
		{  
			perror("mem ftok");
			    exit(1);
		};

		/* creating shared memory */
		if ((mid = shmget(mkey,sizeof(int[15]), 0644 | IPC_EXCL | IPC_CREAT)) == -1)
		{ 
			if ((mid = shmget(mkey,sizeof(int[15]), 0644 | IPC_CREAT)) == -1)
			{
				perror("shmget");
				exit(1);
			}	
		}
		else
		{
			ctr = 1;
		}
	}
	sb.sem_num = 0;
	sb.sem_op = -1;
	sb.sem_flg = 0;

	registered = (int*)shmat(mid,0,0);
	/* puts zero an  array if it was just created */
	if(ctr == 1)
	{
		for(i = 0 ; i < 15;i++)
		{
			registered[i] = 0;
		}
	}
    ctr = 0;
	/* created all needed IPC devices
	 * now we will register user */
	procesid = getpid();	
	
	if (semop(sid, &sb, 1) == -1) 
	{
		perror("semop");
	        exit(1);
	}

	for(i = 0 ; i < 15;i++)
	{
		if(registered[i] == 0 && c == 0)
		{
			registered[i] = procesid;
			c = 1;
		}
	}

	sb.sem_op = 1;
	if (semop(sid, &sb, 1) == -1) 
	{ 
		perror("semop");  
       		exit(1);
 	}

	if( c == 0 )
	{
		printf("Chat is being used by maximum users right now, please try again later\n");
		exit(0);
	}

	/* creating username */
	userid = getuid();
	pwd = getpwuid(userid);	
	sprintf(newpid, "%d", (int)procesid);;
	strcpy(buf.usrname,pwd -> pw_name);
	strcat(buf.usrname,newpid);
	strcat(buf.usrname,"\0");
	
	/* chat starts */
	/* forking to make both reading and writin at the 'same time' posible */
	if( ( child = fork() ) < 0 ) 
		perror("Fork:\n");
	else if ( child == 0 )
	{
		
		 for(;;)
		 {
			/* reciving msg */ 
		 	if (msgrcv(qid, (struct msgbuf *)&bufrcv, sizeof(bufrcv), procesid, 0) == -1) 
		 	{
               			perror("msgrcv");
           		 	exit(1);
               	 	}
        	 	printf("%s: %s\n", bufrcv.usrname, bufrcv.mtext);	
		 }
	}
	else
	{	 
		for(;;)	
		{
			/* sending msg*/
			if (fgets(buf.mtext, MAXLINE, stdin) != NULL ) 
  	 		{	
				if(buf.mtext[0] == '\n')
					continue;
				if(buf.mtext[0] == ':' && buf.mtext[1] == 'q')
					break;;				
				sb.sem_op = -1;
    				if (semop(sid, &sb, 1) == -1) 
				{
       					perror("semop");
       					exit(1);
   				}
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

				sb.sem_op = 1;
    				if (semop(sid, &sb, 1) == -1) 
				{
         				perror("semop");
        				exit(1);
    				}	 
			}
			else
			{
				printf("Przerwanie przez CTRL + D\n");
					break;
			}
		}
	}

	/* unregistering */
	sb.sem_op = -1;
    	if (semop(sid, &sb, 1) == -1) 
	{
      		perror("semop");
        	exit(1);
   	}

	for( i = 0 ; i < 15 ; i++)    
	{
		if( registered[i] == procesid)
			registered[i] = 0;
	}

	/* checking if IPC devices should be deleted */
	ctr = 0;
	for( i = 0 ; i < 15 ; i++)    
	{
		if(registered[i])
			ctr = 1;
	}

	sb.sem_op = 1;
   	if (semop(sid, &sb, 1) == -1) 	
	{
        	perror("semop");
       		exit(1);
    	}

	if(ctr == 0)
	{
		/* deleting IPC devices */	
		/* deleting semaphore */	
		if (semctl(sid, 0, IPC_RMID, arg) == -1) 
		{
        		perror("semctl");
        		exit(1);
		}

		/* deleting msgq */
		if (msgctl(qid, IPC_RMID, NULL) == -1) 
		{
	        	perror("msgctl");
		        exit(1);
		}

		/* deleting shered memory */
		if (shmctl(mid, IPC_RMID, NULL) == -1)
		{
	        	perror("shmctl");
		        exit(1);
		}
	}
	else
	{
		/* detaching shered memory */
		shmdt((void *) registered);
	}
	return 0;
}

void signalc(int s)
{
	printf("\nPrzerwanie przez CTRL + C\n");
	exit(2);
}

void help()
{
	printf("Program mozna wywolac z opcjami: \n");
	printf("-m shmid \n");
	printf("-q msqid \n");
	printf("-s semid \n");
	printf("Gdzie podajemy urzadzenie IPC do, ktorego chcemy sie podlaczyc.\n");
	printf("Lub bez opcji aby program sam wybral uzadzenia domyslne.\n");
	printf("Aby wyjsc z programu nalezy wpisac :q jako wiadomosc\n");	
}

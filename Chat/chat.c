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

/* veriables made global to allow cleanup */

/* id veriables for message queue, semaphore, shared memory */	
int qid, sid, mid;

/* array of registered users */	
int *registered;

/* semaphore veriable*/
union semun arg;
struct sembuf sb;

/* veriable used to register proces into chat */	
pid_t procesid;

/* functions */

/* function to support ctrl+c */
void signalc(int s);

/* function which displays help if -h argument got used and :h in app itself */
void help_long();
void help_short();

/* function to display welcome msg */
void hello();

/* function to cleanup on exit */
void cleanup();

int main(int argc, char **argv)
{
	
	/* flags to control arguments input */
	int flagmsq = 0,flagshm = 0,flagsem = 0;
	
	/* keys variables used to create ids of veriables above */
	key_t qkey, skey, mkey;
	
	/* struct for sending and reciving msg*/
	struct my_msgbuf buf;	
	struct my_msgbuf bufrcv;
		
	/* veriables used to register user */
	uid_t userid;
	struct passwd *pwd;
	char newpid[6] = {0};
	
	/* iterators, controlers veriables */
	int i = 0,control = 0,ctr = 0;
	
	/* veriable for forking */
	pid_t child;

	/* veriables for signal control */
    	sigset_t iset;
    	struct sigaction act;
	
	/* option variable */
	int opt;

	if ( argc > 1 )
	{
		while ( (opt = getopt(argc, argv, "m:q:s:h") ) != -1 ) 
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
					help_long();
					exit(0);	
				default:
                   			fprintf(stderr, "Usage: %s [-m shmid] [-q msqid] [-s semid] [-h]\n",argv[0]);
                   			exit(1);
               		}
           	}
	}

	/* control of CTRL + C */
    	sigemptyset(&iset);
    	act.sa_handler = &signalc;
    	act.sa_mask = iset;
	act.sa_flags = 0;
    	sigaction(SIGINT, &act, NULL);

	/* creating all needed IPC devices */
	if ( flagmsq == 0 )
	{

		/* obtaining msq key */
		if( (qkey = ftok("/etc/passwd", '1') ) == -1 ) 
		{  
			perror("msq ftok");
			exit(2);
		};

		/* connecting or creating a message queue */
		if ( (qid = msgget(qkey, 0644 | IPC_CREAT) ) == -1 ) 
		{
			perror("msgget");
			exit(2);
		};
	}

	if ( flagsem == 0 )
	{
		/* obtaining sem key */
		if( (skey = ftok("/etc/passwd", '2') ) == -1 ) 
		{  
			perror("sem ftok");
			exit(2);
		};

		/* creating or connecting semaphore */
		if ( (sid = semget(skey, 1 ,0666 | IPC_EXCL | IPC_CREAT) ) == -1 ) 
		{	
			if ( (sid = semget(skey, 1 , 0666 |IPC_CREAT) ) == -1 ) 
			{
				perror("semget");
				exit(2);		
			}
		}
		else
		{	
			arg.val = 1;
			if ( semctl(sid, 0, SETVAL, arg) == -1 ) 
			{
		   		perror("semctl");
		    		exit(2);
			}	
		}
	}
	
	/* seting a semaphore */
	sb.sem_num = 0;
	sb.sem_op = -1;
	sb.sem_flg = 0;
	
	if ( flagshm == 0 )
	{
		/* obtaining shm key */
		if( (mkey = ftok("/etc/passwd", '3') ) == -1 ) 
		{  
			perror("mem ftok");
			exit(2);
		};

		/* creating or connecting shared memory */
		if ( (mid = shmget(mkey,sizeof(int[15]), 0644 | IPC_EXCL | IPC_CREAT) ) == -1 )
		{ 
			if ( (mid = shmget(mkey,sizeof(int[15]), 0644 | IPC_CREAT) ) == -1 )
			{
				perror("shmget");
				exit(2);
			}	
		}
		else
		{
			ctr = 1;
		}
	}

	/* maping an shm segment */
	registered = (int*)shmat(mid,0,0);

	/* puts zero an  array if it was just created */
	if ( ctr == 1 )
	{
		for ( i = 0 ; i < 15;i++ )
		{
			registered[i] = 0;
		}
	}

	/* created all needed IPC devices */
	/* now we will register user */
	procesid = getpid();	
	
	if ( semop(sid, &sb, 1) == -1 ) 
	{
		perror("semop");
	        exit(2);
	}

	for ( i = 0 ; i < 15;i++ )
	{
		if ( registered[i] == 0 && control == 0 )
		{
			registered[i] = procesid;
			control = 1;
		}
	}

	sb.sem_op = 1;
	if (semop(sid, &sb, 1) == -1 ) 
	{ 
		perror("semop");  
       		exit(2);
 	}

	if ( control == 0 )
	{
		printf("Chat is being used by maximum users right now, please try again later\n");
		exit(0);
	}

	/* creating username */
	userid = getuid();
	pwd = getpwuid(userid);	
	strcpy(buf.usrname,pwd -> pw_name);
	sprintf(newpid, "%d", (int)procesid);
	strcat(buf.usrname,newpid);
	strcat(buf.usrname,"\0");

	/* displaying hello and help */
	hello();	
	help_short();
		
	/* chat starts */
	/* forking to make both reading and writin at the 'same time' posible */
	if( ( child = fork() ) < 0 ) 
	{
		perror("fork");
		exit(2);
	}
	else if ( child == 0 )
	{
		 for(;;)
		 {
			/* reciving msg */ 
		 	if ( msgrcv(qid, (struct msgbuf *)&bufrcv, sizeof(bufrcv), procesid, 0) == -1 ) 
		 	{
               			perror("msgrcv");
           		 	exit(2);
               	 	}

        	 	printf("%s: %s\n", bufrcv.usrname, bufrcv.mtext);	
		 }
	}
	else
	{	 
		for(;;)	
		{
			/* sending msg*/
			if ( fgets(buf.mtext, MAXLINE, stdin) != NULL ) 
  	 		{	
				if ( buf.mtext[0] == '\n' )
					continue;
				else if ( buf.mtext[0] == ':' && buf.mtext[1] == 'q' )
					break;
				else if ( buf.mtext[0] == ':' && buf.mtext[1] == 'h' )
				{
					help_short();
					continue;
				}

				sb.sem_op = -1;
    				if ( semop(sid, &sb, 1) == -1 ) 
				{
       					perror("semop");
       					exit(2);
   				}

				for ( i = 0 ; i < 15 ; i++ )    
				{
					if ( registered[i] == 0 )
						continue;
					else
					{		
						buf.mtype = registered[i];	
						if ( msgsnd(qid,(struct msgbuf *)&buf, sizeof(buf),0) == -1 )
							perror("msgsnd");
					}
				}

				sb.sem_op = 1;
    				if ( semop(sid, &sb, 1) == -1 ) 
				{
         				perror("semop");
        				exit(2);
    				}	 
			}
			else
			{
				printf("Przerwanie przez CTRL + D\n");
				cleanup();
				break;
			}
		}
	}
	cleanup();
	return 0;
}
void cleanup()
{
	/* iterator and control variable */
	int i, ctr = 0;

	/* unregistering */
	sb.sem_op = -1;
    	if ( semop(sid, &sb, 1) == -1 ) 
	{
      		perror("semop");
        	exit(2);
   	}

	for( i = 0 ; i < 15 ; i++ )    
	{
		if( registered[i] == procesid )
			registered[i] = 0;
	}

	/* checking if IPC devices should be deleted */
	for( i = 0 ; i < 15 ; i++ )    
	{
		if( registered[i] )
			ctr = 1;
	}

	sb.sem_op = 1;
   	if ( semop(sid, &sb, 1) == -1 ) 	
	{
        	perror("semop");
       		exit(2);
    	}

	if ( ctr == 0 )
	{
		/* deleting IPC devices */	
		/* deleting semaphore */	
		if ( semctl(sid, 0, IPC_RMID, arg) == -1 ) 
		{
        		perror("semctl");
        		exit(2);
		}

		/* deleting msgq */
		if ( msgctl(qid, IPC_RMID, NULL) == -1 ) 
		{
	        	perror("msgctl");
		        exit(2);
		}

		/* deleting shered memory */
		if ( shmctl(mid, IPC_RMID, NULL) == -1 )
		{
	        	perror("shmctl");
		        exit(2);
		}
	}
	else
	{
		/* detaching shered memory */
		shmdt((void *) registered);
	}
}
void signalc(int s)
{
	printf("\nPrzerwanie przez CTRL + C\n");
	cleanup();
	exit(3);
}

void help_long()
{
	printf("Program mozna wywolac z opcjami: \n");
	printf("-m shmid \n");
	printf("-q msqid \n");
	printf("-s semid \n");
	printf("Gdzie podajemy urzadzenie IPC do, ktorego chcemy sie podlaczyc.\n");
	printf("Lub bez opcji aby program sam wybral uzadzenia domyslne.\n");
	printf("Aby wyjsc z programu nalezy wpisac :q jako wiadomosc\n");	
}

void help_short()
{
	printf("Uzyj:\n:q aby wyjsc\n:h aby wyswietlic pomoc\n\n");
}

void hello()
{
	printf("Witaj w chacie stworzonym przez mtracewicz.\n");
	printf("Chat obluguje do 15 uzytkownikow\n");
}

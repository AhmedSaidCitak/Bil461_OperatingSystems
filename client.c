#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>

#define MSGSZ 1024

typedef struct msgbuf 
	{
		long mtype;
		char mtext[MSGSZ];
	} message;

int main(int argc, char const *argv[])
{

//-----------MATRIS CONTROL-----------

	FILE *in_m1, *in_m2;
	int row_m1, column_m1, row_m2, column_m2;

	in_m1 = fopen(argv[1], "r");
	in_m2 = fopen(argv[2], "r");

	fscanf(in_m1, "%d %d", &row_m1, &column_m1);
	fscanf(in_m2, "%d %d", &row_m2, &column_m2);

	int matrix1[row_m1][column_m1];
	int matrix2[row_m2][column_m2];

	for (int i = 0; i < row_m1; ++i)
		for (int j = 0; j < column_m1; ++j)		
			fscanf(in_m1, "%d", &matrix1[i][j]);

	for (int i = 0; i < row_m2; ++i)
		for (int j = 0; j < column_m2; ++j)
			fscanf(in_m2, "%d", &matrix2[i][j]);	

	if(column_m1 != row_m2)
	{
		printf("%s\n", "Matrices cannot be multiplied!");
		exit(0);
	}

	int result[row_m1][column_m2];

//----------SENDING PID THROUGH PUBLIC MESSAGE QUEUE----------

	key_t key;
	size_t buf_length;
	int qId, pid_client;
	message qbuf;

	pid_client = getpid();

	if((key = ftok("/tmp", 'C')) == -1)
	{
        perror("CLIENT: public queue ftok failed!");
        exit(1);
    }

    if((qId = msgget(key, IPC_CREAT | 0666)) == -1)
    {
        perror("CLIENT: Failed to create public message queue!");
        exit(2);
    }

    qbuf.mtype = 1;
    sprintf(qbuf.mtext, "%d", pid_client);

    buf_length = strlen(qbuf.mtext) + 1;

    if((msgsnd(qId, &qbuf, buf_length, IPC_NOWAIT)) < 0)
    {
		perror("CLIENT: msgsnd function failed in public message queue!");
		exit(1);
	}

//--------SENDING MATRIX DIMENSIONS THROUGH PRIVATE MESSAGE QUEUE-------

	message priv_qbuf;

	if((key = ftok("/tmp", pid_client)) == -1)
	{
        perror("CLIENT: private queue ftok failed!");
        exit(1);
    }

    if((qId = msgget(key, IPC_CREAT | 0666)) == -1)
    {
        perror("CLIENT: Failed to create private message queue!");
        exit(2);
    }

    priv_qbuf.mtype = 1;
    sprintf(priv_qbuf.mtext, "%d %d %d %d", row_m1, column_m1, row_m2, column_m2);
    buf_length = strlen(priv_qbuf.mtext) + 1;

    if((msgsnd(qId, &priv_qbuf, buf_length, IPC_NOWAIT)) < 0)
    {
		perror("CLIENT: msgsnd function failed in public message queue!");
		exit(1);
	}

	sleep(5);

//--------IN ORDER TO WRITE SHMEM CLIENT IS WAITING FOR SERVER TO CREATE SHARED MEMORY---------- 

	if (msgrcv(qId, &priv_qbuf, MSGSZ, 1, 0) < 0)
	{
		perror("CLIENT: msgrcv function failed!");
		exit(1);
	}
	else
	{
		int shmid;
		sscanf(priv_qbuf.mtext, "%d", &shmid);
		int *shm_ptr = (int *) shmat(shmid, NULL, 0);
		int *shm_ptr2 = shm_ptr;


		for (int i = 0; i < row_m1; ++i)
		{
			for (int j = 0; j < column_m1; ++j)
			{
				*shm_ptr = matrix1[i][j];
				shm_ptr += sizeof(int);
			}
		}

		for (int i = 0; i < row_m2; ++i)
		{	
			for (int j = 0; j < column_m2; ++j)
			{	
				*shm_ptr = matrix2[i][j];
				shm_ptr += sizeof(int);
			}
		}

		if((msgsnd(qId, &priv_qbuf, buf_length, IPC_NOWAIT)) < 0)
	    {
			perror("CLIENT: msgsnd function failed in public message queue!");
			exit(1);
		}

		sleep(5);

//---------MATRICES ARE IN SHARED MEMORY-----------	

		if(msgrcv(qId, &priv_qbuf, MSGSZ, 1, 0) < 0)
		{
			perror("CLIENT: msgrcv function failed!");
			exit(1);
		}
		else
		{
			for (int i = 0; i < row_m1; ++i)
			{
				for (int j = 0; j < column_m1; ++j)
				{
					shm_ptr2 += sizeof(int);
				}
			}

			for (int i = 0; i < row_m2; ++i)
			{
				for (int j = 0; j < column_m2; ++j)
				{
					shm_ptr2 += sizeof(int);
				}
			}

			for (int i = 0; i < row_m1; ++i)
			{
				for (int j = 0; j < column_m2; ++j)
				{
					printf("%d ", *shm_ptr2);
					shm_ptr2 += sizeof(int);
				}
				printf("\n");
			}
		}	
	}
}
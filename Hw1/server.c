#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

#define MSGSZ 1024

typedef struct msgbuf 
{
	long mtype;
	char mtext[MSGSZ];
} message;

void *threadFunction(int pid)
{
    message priv_qbuf;
    key_t key;
    int qId;
    int row_m1, column_m1, row_m2, column_m2;

	if((key = ftok("/tmp", pid)) == -1)
	{
        perror("SERVER: private queue ftok failed!");
        exit(1);
    }

    if((qId = msgget(key, IPC_CREAT | 0666)) == -1)
    {
        perror("SERVER: Failed to create private message queue!");
        exit(2);
    }

    if (msgrcv(qId, &priv_qbuf, MSGSZ, 1, 0) < 0)
	{
		perror("SERVER: msgrcv function failed!");
		exit(1);
	}
	else
	{
		sscanf(priv_qbuf.mtext, "%d %d %d %d", &row_m1, &column_m1, &row_m2, &column_m2);

		int shmid = shmget(key, sizeof(int) * (row_m1 * column_m1 + row_m2 * column_m2 + row_m1 * column_m2), IPC_CREAT | 0666);
		int *shm_ptr = (int *) shmat(shmid, NULL, 0);
		int *shm_ptr2 = shm_ptr;

		priv_qbuf.mtype = 1;
	    sprintf(priv_qbuf.mtext, "%d", shmid);
	    int buf_length = strlen(priv_qbuf.mtext) + 1;

	    msgsnd(qId, &priv_qbuf, buf_length, IPC_NOWAIT);
	    sleep(5);

//--------------SHARED MEMORY CREATED-------------- 	    

	    if (msgrcv(qId, &priv_qbuf, MSGSZ, 1, 0) < 0)
		{
			perror("SERVER: msgrcv function failed!");
			exit(1);
		}
		else
		{
			int matrix1[row_m1][column_m1];
	    	int matrix2[row_m2][column_m2];
	    	int result[row_m1][column_m2];

			for (int i = 0; i < row_m1; ++i)
		    {
				for (int j = 0; j < column_m1; ++j)
				{
					matrix1[i][j] = *shm_ptr;
					shm_ptr += sizeof(int);
				}
		    }

			for (int i = 0; i < row_m2; ++i)
			{
				for (int j = 0; j < column_m2; ++j)
				{
					matrix2[i][j] = *shm_ptr;
					shm_ptr += sizeof(int);
				}
			}

			for (int i = 0; i < row_m1; ++i)
			{
				for (int j = 0; j < column_m2; ++j)
				{
					result[i][j] = 0;
					for (int k = 0; k < column_m1; ++k)
					{
						result[i][j] += matrix1[i][k] * matrix2[k][j];
					}
				}
			}

			for (int i = 0; i < row_m1; ++i)
			{
				for (int j = 0; j < column_m1; ++j)
				{
					*shm_ptr2 = matrix1[i][j];
					shm_ptr2 += sizeof(int);
				}
			}

			for (int i = 0; i < row_m2; ++i)
			{	
				for (int j = 0; j < column_m2; ++j)
				{	
					*shm_ptr2 = matrix2[i][j];
					shm_ptr2 += sizeof(int);
				}
			}

			for (int i = 0; i < row_m1; ++i)
			{	
				for (int j = 0; j < column_m2; ++j)
				{	
					*shm_ptr2 = result[i][j];
					shm_ptr2 += sizeof(int);
				}
			}

			msgsnd(qId, &priv_qbuf, buf_length, IPC_NOWAIT);

			sleep(5);

			shmdt((void*) shm_ptr);
			shmdt((void*) shm_ptr2);
			pthread_exit(NULL);
		}
	}

//----------MATRIX MULT MADE------- 
	
}

int main(int argc, char const *argv[])
{

	key_t key;
	int qId;
	message qbuf, priv_qbuf;

	if((key = ftok("/tmp", 'C')) == -1)
	{
        perror("SERVER: ftok failed!");
        exit(1);
    }

    if((qId = msgget(key, IPC_CREAT | 0666)) == -1)
    {
        perror("SERVER: Failed to create message queue!");
        exit(2);
    }

	while(1)
	{
		if (msgrcv(qId, &qbuf, MSGSZ, 1, 0) < 0)
		{
			perror("SERVER: msgrcv function failed!");
			exit(1);
		}
		else
		{
			int pid;
			pthread_t thread_id;
			sscanf(qbuf.mtext, "%d", &pid);
		    pthread_create(&thread_id, NULL, threadFunction, pid);
		}
	}
}
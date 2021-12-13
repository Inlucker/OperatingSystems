#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>

#define ACTIVE_READERS 0
#define ACTIVE_WRITER 1
#define CAN_READ 2
#define CAN_WRITE 3

#define ITERATIONS 10

int semid;
int *value;

int getVal(int semnum)
{
	return semctl(semid, semnum, GETVAL, 0);
}

int printGetAll()
{
	ushort array[4] = {0, 0, 0, 0}; 
	int rtrn = semctl(semid, 0, GETALL, array);
	for (int i = 0; i < 4; i++)
		printf ("%d ", array[i]); 
	printf("\n");
	return rtrn;
}

int printVal()
{
	if (value)
	{
		printf("Value = %d\n", *value);
		return 1;
	}
	else
		return 0;
}

struct sembuf start_read[5] =
{
	{CAN_READ, 1, 0},
	{ACTIVE_WRITER, 0, 0},
	{CAN_WRITE, 0, 0},
	{ACTIVE_READERS, 1, 0},
	{CAN_READ, -1, 0}
};

/*struct sembuf started_reading[2] =
{
	{ACTIVE_READERS, 1, 0},
	{CAN_READ, -1, 0}
};*/

struct sembuf stop_read[1] =
{
	{ACTIVE_READERS, -1, 0}
};

void reader(int r_id, int delay)
{
	//delay = 1000000;
	//printf("Читатель %d delay = %d\n", prod_id, delay);
	for (int j = 0; j < ITERATIONS; j++)
	{
		usleep(delay);
		
		/*if (semop(semid, start_read, 3) == -1)
		{
			perror("start_read semop error\n");
			exit(1);
		}
		
		printf("Читатель %d начал читать\n", r_id);
		
		if (semop(semid, started_reading, 2) == -1)
		{
			perror("started_reading semop error\n");
			exit(1);
		}*/
		
		if (semop(semid, start_read, 5) == -1)
		{
			perror("start_read semop error\n");
			exit(1);
		}
		
		printf("Читатель %d начал читать\n", r_id);
		
		printf("Читатель %d считал значение: %d\n", r_id, *value);
		
		if (semop(semid, stop_read, 1) == -1)
		{
			perror("stop_read semop error\n");
			exit(1);
		}	
		printf("Читатель %d перестал читать\n", r_id);
	}
}

struct sembuf start_write[5] =
{
	{CAN_WRITE, 1, 0},
	{ACTIVE_READERS, 0, 0},
	{ACTIVE_WRITER, 0, 0},
	{ACTIVE_WRITER, 1, 0},
	{CAN_WRITE, -1, 0}
};

struct sembuf stoped_writing[1] =
{
	{ACTIVE_WRITER, -1, 0}
};


void writer(int r_id, int delay)
{
	//delay = 5000000;
	//printf("Писатель %d delay = %d\n", prod_id, delay);
	for (int j = 0; j < ITERATIONS; j++)
	{
		usleep(delay);
		
		if (semop(semid, start_write, 5) == -1)
		{
			perror("start_write semop error\n");
			exit(1);
		}
		
		printf("Писатель %d начал писать\n", r_id);
		
		printf("Писатель %d записал значение: %d\n", r_id, ++*value);
		
		if (semop(semid, stoped_writing, 1) == -1)
		{
			perror("stoped_writing semop error\n");
			exit(1);
		}	
		
		printf("Писатель %d перестал писать\n", r_id);			
	}
}

int createChild(int cid, void (*fptr)(int, int))
{
	int delay = rand()%(3000000) + 1000000;
	int childpid;
	if ((childpid = fork()) == -1)
	{
		perror("fork error\n");
		return EXIT_FAILURE;
	}
	else if (childpid == 0)
	{
		fptr(cid, delay);
		exit(EXIT_SUCCESS);
	}
}

int main()
{	
	int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	//Создаем набор из 4 семафоров
	semid = semget(IPC_PRIVATE, 4, IPC_CREAT | perms);
    if (semid == -1)
    {
        perror("semget error\n");
        return 1;
    }
	
	//Инициализация семафоров //Необязательно
	ushort array[4]= {0, 0, 0, 0}; 
	if (semctl(semid, 0, SETALL, array) == -1)
    {
        perror("semctl SETALL error\n");
        return 1;
    }
	
	/*printf("Sems at start: ");
	if (printGetAll(semid) == -1)
	{
        perror("semctl GETALL error\n");
        return 1;
	}*/
	
	int shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | perms);
	if (shmid == -1)
	{
		perror("shmget error\n");
        return 1;
	}
	value = shmat(shmid,NULL,0);
	if (value == (void*)-1)
	{
		perror("shmat error\n");
		return 1;
	}
	
	*value = 0; //Необязательно
	
	//Создаём читателей и писателей
	for (int i = 0; i < 3; i++)
	{
		createChild(i+1, &reader);
		createChild(i+1, &writer);	
	}
	
	int status;
	
	for (int i = 0; i < 3*2; i++)
	{
		wait(&status);
		/*if (!printBuf())
		{
			perror("buffer == NULL");
			return 1;
		}*/
	}	

	printf("ended\n");
    return 0;
}
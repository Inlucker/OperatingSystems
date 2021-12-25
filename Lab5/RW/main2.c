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
#define BIN_SEM 1
//#define CAN_READ 2
#define CAN_WRITE 2

#define ITERATIONS 10

int semid;
int *value;

int getVal(int semnum)
{
	return semctl(semid, semnum, GETVAL, 0);
}

int printGetAll()
{
	ushort array[3] = {0, 0, 0}; 
	int rtrn = semctl(semid, 0, GETALL, array);
	for (int i = 0; i < 3; i++)
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

struct sembuf start_read[4] =
{
	//{CAN_READ, 1, 0},
	{BIN_SEM, -1, 0},
	{BIN_SEM, 1, 0},
	{CAN_WRITE, 0, 0},
	{ACTIVE_READERS, 1, 0}
	//{CAN_READ, -1, 0}
};

struct sembuf stop_read[1] =
{
	{ACTIVE_READERS, -1, 0}
};

void reader(int r_id, int delay)
{
	//delay*=10;
	//delay = 1000000;
	//printf("Читатель %d delay = %d\n", r_id, delay);
	for (int j = 0; j < ITERATIONS; j++)
	{
		usleep(delay);
		
		if (semop(semid, start_read, 4) == -1)
		{
			perror("start_read semop error\n");
			exit(1);
		}
		
		//printf("Читатель %d начал читать\n", r_id);
		
		printf("Читатель %d считал значение: %d\n", r_id, *value);
		//usleep(delay*10);
		
		if (semop(semid, stop_read, 1) == -1)
		{
			perror("stop_read semop error\n");
			exit(1);
		}
		//printGetAll();
		//printf("Читатель %d перестал читать\n", r_id);
	}
	//printf("Читатель %d заврешился\n", r_id);
	//printGetAll();

}

struct sembuf start_write[4] =
{
	{CAN_WRITE, 1, 0},
	{ACTIVE_READERS, 0, 0},
	{BIN_SEM, -1, 0},
	{CAN_WRITE, -1, 0}
};

struct sembuf stoped_writing[1] =
{
	{BIN_SEM, 1, 0}
};


void writer(int w_id, int delay)
{
	//delay*=10;
	//delay = 5000000;
	//printf("Писатель %d delay = %d\n", w_id, delay);
	for (int j = 0; j < ITERATIONS; j++)
	{
		usleep(delay);
		
		if (semop(semid, start_write, 4) == -1)
		{
			perror("start_write semop error\n");
			exit(1);
		}
		
		//printf("Писатель %d начал писать\n", w_id);
		
		printf("Писатель %d записал значение: %d\n", w_id, ++*value);
		//usleep(delay*10);
		
		if (semop(semid, stoped_writing, 1) == -1)
		{
			perror("stoped_writing semop error\n");
			exit(1);
		}
		//printGetAll();
		
		//printf("Писатель %d перестал писать\n", w_id);			
	}
	//printf("Писатель %d заврешился\n", w_id);
	//printGetAll();
}

int createChild(int cid, void (*fptr)(int, int))
{
	int delay = rand()%(30000) + 10000;
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
	srand(time(NULL));
	int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	//Создаем набор из 3 семафоров
	semid = semget(IPC_PRIVATE, 3, IPC_CREAT | perms);
    if (semid == -1)
    {
        perror("semget error\n");
        return 1;
    }
	
	//Инициализация семафоров //Необязательно
	ushort array[3]= {0, 1, 0}; 
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
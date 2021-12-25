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
#define READERS_QUEUE 2
#define WRITERS_QUEUE 3
#define CAN_READ 4 // 0 = FALSE, 1 = TRUE
#define CAN_WRITE 5 // 0 = FALSE, 1 = TRUE

#define ITERATIONS 10

int semid;
int *value;

int getVal(int semnum)
{
	return semctl(semid, semnum, GETVAL, 0);
}

int printGetAll()
{
	ushort array[6] = {0, 0, 0, 0, 0, 0}; 
	int rtrn = semctl(semid, 0, GETALL, array);
	//for (int i = 0; i < 6; i++)
		//printf ("%d ", array[i]); 
	printf("ACTIVE_READERS = %d\n", array[0]);
	printf("ACTIVE_WRITER  = %d\n", array[1]);
	printf("READERS_QUEUE  = %d\n", array[2]);
	printf("WRITERS_QUEUE  = %d\n", array[3]);
	printf("CAN_READ       = %d\n", array[4]);
	printf("CAN_WRITE      = %d\n", array[5]);
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

//READ STRUCTS
struct sembuf queue_read[1] =
{
	{READERS_QUEUE, 1, 0}
};

struct sembuf wait_can_read[1] =
{
	{CAN_READ, -1, 0}

};

struct sembuf start_read[2] =
{
	{ACTIVE_READERS, 1, 0},
	{READERS_QUEUE, -1, 0}
};

struct sembuf stop_read[1] =
{
	{ACTIVE_READERS, -1, 0}
};

struct sembuf signal_can_read[1] =
{
	{CAN_READ, 1, 0}
};

//WRITE STRUCTS
struct sembuf queue_write[1] =
{
	{WRITERS_QUEUE, 1, 0}
};

struct sembuf wait_can_write[1] =
{
	{CAN_WRITE, -1, 0}
};

struct sembuf start_write[2] =
{
	{ACTIVE_WRITER, 1, 0},
	{WRITERS_QUEUE, -1, 0}
};

struct sembuf stop_write[1] =
{
	{ACTIVE_WRITER, -1, 0}
};

struct sembuf signal_can_write[1] =
{
	{CAN_WRITE, 1, 0}
};

void startRead()
{
	if (semop(semid, queue_read, 1) == -1)
	{
		perror("queue_read semop error\n");
		exit(1);
	}

	if (getVal(ACTIVE_WRITER) || getVal(WRITERS_QUEUE))
	{
		if (semop(semid, wait_can_read, 1) == -1)
		{
			perror("wait_can_read semop error\n");
			exit(1);
		}
	}

	if (semop(semid, start_read, 2) == -1)
	{
		perror("start_read semop error\n");
		exit(1);
	}

	if (getVal(CAN_READ) == 0)
		if (semop(semid, signal_can_read, 1) == -1)
		{
			perror("signal_can_read semop error\n");
			exit(1);
		}
}

void stopRead()
{
	if (semop(semid, stop_read, 1) == -1)
	{
		perror("stop_read semop error\n");
		exit(1);
	}
	if (getVal(ACTIVE_READERS) == 0)
	{
		if (!getVal(CAN_WRITE))
			if (semop(semid, signal_can_write, 1) == -1)
			{
				perror("signal_can_write semop error\n");
				exit(1);
			}
	}
}

void reader(int r_id, int delay)
{
	//delay *= 10;
	//delay += 100000;
	printf("Читатель %d delay = %d\n", r_id, delay);
	for (int j = 0; j < ITERATIONS; j++)
	{
		usleep(delay);
		
		startRead();
		
		//printf("Читатель %d начал читать\n", r_id);
		
		//printGetAll();
		printf("Читатель %d считал значение: %d\n", r_id, *value);
		//usleep(delay*10); //INFINITE ACTIVE_READERS
		
		
		stopRead();
		
		//printf("Читатель %d перестал читать\n", r_id);
	}
}

void startWrite()
{
	if (semop(semid, queue_write, 1) == -1)
	{
		perror("queue_write semop error\n");
		exit(1);
	}
	if (getVal(ACTIVE_READERS) || getVal(ACTIVE_WRITER))
	{
		if (semop(semid, wait_can_write, 1) == -1)
		{
			perror("wait_can_write semop error\n");
			exit(1);
		}
		
	}

	if (semop(semid, start_write, 2) == -1)
	{
		perror("start_write semop error\n");
		exit(1);
	}
}

void stopWrite()
{
	if (semop(semid, stop_write, 1) == -1)
	{
		perror("stop_write semop error\n");
		exit(1);
	}

	if (getVal(READERS_QUEUE))
	{
		if (getVal(CAN_READ) == 0)
			if (semop(semid, signal_can_read, 1) == -1)
			{
				perror("signal_can_read semop error\n");
				exit(1);
			}
	}
	else
	{
		if (!getVal(CAN_WRITE))
			if (semop(semid, signal_can_write, 1) == -1)
			{
				perror("signal_can_write semop error\n");
				exit(1);
			}
	}
}

void writer(int w_id, int delay)
{
	//delay *= 10;
	//delay += 100000;
	printf("Писатель %d delay = %d\n", w_id, delay);
	for (int j = 0; j < ITERATIONS; j++)
	{
		usleep(delay);

		startWrite();
		
		//printf("Писатель %d начал писать\n", w_id);
		
		printf("Писатель %d записал значение: %d\n", w_id, ++*value);
		//usleep(delay*100); //INFINITE ACTIVE_WRITERS
		
		
		stopWrite();
		
		
		//printf("Писатель %d перестал писать\n", w_id);			
	}
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
	//Создаем набор из 6 семафоров
	semid = semget(IPC_PRIVATE, 6, IPC_CREAT | perms);
    if (semid == -1)
    {
        perror("semget error\n");
        return 1;
    }
	
	//Инициализация семафоров //Необязательно
	ushort array[6]= {0, 0, 0, 0, 0, 0}; 
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
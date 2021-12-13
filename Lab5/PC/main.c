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

#define BUFFER_FULL 0
#define BUFFER_EMPTY 1
#define BIN_SEM 2

#define BUFFER_SIZE 10

#define ITERATIONS 3

int semid;
char *buffer;

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

int printBuf()
{
	if (buffer)
	{
		printf("Buffer = ");
		for (int i = 0; i < BUFFER_SIZE; i++)
			printf("%c", buffer[i]);
		//printf("_%c", buffer[BUFFER_SIZE]);
		printf("\n");
		return 1;
	}
	else
		return 0;
}

//P (proberen – проверять)
struct sembuf ProducerP[2] =
{
	{BUFFER_EMPTY, -1, 0},
	{BIN_SEM, -1, 0}
};

//V (verhogen – увеличивать)
struct sembuf ProducerV[2] =
{
	{BUFFER_FULL, 1, 0},
	{BIN_SEM, 1, 0}
};

void producer(int prod_id, int delay)
{
	//printf("Производитель %d delay = %d\n", prod_id, delay);
	for (int j = 0; j < ITERATIONS; j++)
	{
		usleep(delay);
		
		if (semop(semid, ProducerP, 2) == -1)
		{
			perror("ProducerP semop error\n");
			exit(1);
		}
		
		/*printf("Sems after ProducerP %d: ", prod_id);
		if (printGetAll(semid) == -1)
		{
			perror("semctl GETALL error\n");
			exit(1);
		}*/
		
		printf("Производитель %d в критической секции\n", prod_id);
		
		int full = getVal(BUFFER_FULL);
		//printf("BUFFER_FULL = %d\n", full);
		if (full != -1)
		{
			buffer[full] = buffer[BUFFER_SIZE];
			printf("Производитель %d записал '%c'\n", prod_id, buffer[BUFFER_SIZE]);
			for (int i = full; i > 0; i--)
			{
				buffer[i] = buffer[i-1];
			}
			buffer[0] =  buffer[BUFFER_SIZE]++;
		}
		else
			printf("Производитель %d не смог записать\n", prod_id);	
		
		/*if (!printBuf())
		{
			perror("buffer == NULL");
			exit(1);
		}*/
		
		if (semop(semid, ProducerV, 2) == -1)
		{
			perror("ProducerV semop error\n");
			exit(1);
		}	
		printf("Производитель %d вышел из критической секции\n", prod_id);
		
		/*printf("Sems after ProducerV %d: ", prod_id);
		if (printGetAll(semid) == -1)
		{
			perror("semctl GETALL error\n");
			exit(1);
		}*/
	}
}

//P (proberen – проверять)
struct sembuf ConsumerP[2] =
{
	{BUFFER_FULL, -1, 0},
	{BIN_SEM, -1, 0}
};

//V (verhogen – увеличивать)
struct sembuf ConsumerV[2] =
{
	{BUFFER_EMPTY, 1, 0},
	{BIN_SEM, 1, 0}
};

void consumer(int cons_id, int delay)
{
	//printf("Потребитель %d delay = %d\n", cons_id, delay+500000);
	for (int j = 0; j < ITERATIONS; j++)
	{
		usleep(delay+900000);
		
		if (semop(semid, ConsumerP, 2) == -1)
		{
			perror("ConsumerP semop error\n");
			exit(1);
		}
		
		/*printf("Sems after ConsumerP %d: ", cons_id);
		if (printGetAll(semid) == -1)
		{
			perror("semctl GETALL error\n");
			exit(1);
		}*/
		
		printf("Потребитель %d в критической секции\n", cons_id);
		
		int full = getVal(BUFFER_FULL);
		if (full != -1)
		{
			printf("Потребитель %d считал '%c'\n", cons_id, buffer[full]);
			buffer[full] = '0';
		}
		else
			printf("Потребитель %d не смог считать\n", cons_id);
		
		/*if (!printBuf())
		{
			perror("buffer == NULL");
			exit(1);
		}*/
		
		if (semop(semid, ConsumerV, 2) == -1)
		{
			perror("ConsumerC semop error\n");
			exit(1);
		}	
		printf("Потребитель %d вышел из критической секции\n", cons_id);
		
		/*printf("Sems after ConsumerC %d: ", cons_id);
		if (printGetAll(semid) == -1)
		{
			perror("semctl GETALL error\n");
			exit(1);
		}*/
	}
}

int createChild(int cid, void (*fptr)(int, int))
{
	int delay = rand()%(4000000) + 1000000;
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
	
	//Инициализация семафоров
	ushort array[3]= {0, BUFFER_SIZE, 1}; 
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
	
	int shmid = shmget(IPC_PRIVATE, BUFFER_SIZE+1, IPC_CREAT | perms);
	if (shmid == -1)
	{
		perror("shmget error\n");
        return 1;
	}
	buffer = shmat(shmid,NULL,0);
	if (buffer == (void*)-1)
	{
		perror("shmat error\n");
		return 1;
	}
	
	/*if (!printBuf())
	{
		perror("buffer == NULL");
		return 1;
	}*/
	
	//Заполним буффер нулями
	if (buffer)
		for (int i = 0; i < BUFFER_SIZE; i++)
			buffer[i] = '0';
	buffer[BUFFER_SIZE] = 'a'; //Чтобы записывать буквы по алфавиту
		
	/*if (!printBuf())
	{
		perror("buffer == NULL");
		return 1;
	}*/
	
	//Создаём производителей и потребителей
	for (int i = 0; i < 3; i++)
	{
		createChild(i+1, &producer);
		createChild(i+1, &consumer);	
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

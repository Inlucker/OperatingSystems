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
		//printf("_%d", buffer[BUFFER_SIZE+1]);
		printf("\n");
		return 1;
	}
	else
		return 0;
}

//P (proberen – проверять)
struct sembuf ProducerP[2] =
{
	{BUFFER_EMPTY, -1, 0}, // Ожидает освобождения хотя бы одной ячейки буфера.
	{BIN_SEM, -1, 0}  // Ожидает, пока другой производитель или потребитель выйдет из критической зоны.
};

//V (verhogen – увеличивать)
struct sembuf ProducerV[2] =
{
	{BUFFER_FULL, 1, 0},  // Увеличивает кол-во заполненных ячеек.
	{BIN_SEM, 1, 0} // Освобождает критическую зону.
};

void producer(int prod_id, int delay)
{
	//srand(prod_id + time(NULL));
	//delay = rand()%5000000;
	//printf("Производитель %d delay = %d\n", prod_id, delay);
	usleep(delay); //change
	
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
	if (full != -1)
	{
		buffer[full] = buffer[BUFFER_SIZE];
		printf("Производитель %d записал '%c'\n", prod_id, buffer[BUFFER_SIZE]++);
		for (int i = full; i > 0; i--)
		{
			buffer[i] = buffer[i-1];
		}
		buffer[0] =  buffer[BUFFER_SIZE];
	}
	else
		printf("Производитель %d не смог записать\n", prod_id);
	
	//Неправильно
	/*if (full != -1)
	{
		buffer[full] = buffer[BUFFER_SIZE];
		printf("Производитель %d записал '%c'\n", prod_id, buffer[BUFFER_SIZE]++);
	}
	else
		printf("Производитель %d не смог записать\n", prod_id);*/
	
	
	//printf("BUFFER_FULL = %d\n", full);
	
	
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

//P (proberen – проверять)
struct sembuf ConsumerP[2] =
{
	{BUFFER_FULL, -1, 0}, // Ожидает освобождения хотя бы одной ячейки буфера.
	{BIN_SEM, -1, 0}  // Ожидает, пока другой производитель или потребитель выйдет из критической зоны.
};

//V (verhogen – увеличивать)
struct sembuf ConsumerV[2] =
{
	{BUFFER_EMPTY, 1, 0},  // Увеличивает кол-во заполненных ячеек.
	{BIN_SEM, 1, 0} // Освобождает критическую зону.
};

void consumer(int cons_id, int delay)
{
	//srand(cons_id + time(NULL));
	//int delay = rand()%5000000;
	//printf("Потребитель %d delay = %d\n", cons_id, delay+500000);
	usleep(delay+500000); //change
	
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
	
	//Неправильно
	/*printf("Потребитель %d считал '%c'\n", cons_id, buffer[buffer[BUFFER_SIZE+1]]);
	buffer[buffer[BUFFER_SIZE+1]] = '0';
	buffer[BUFFER_SIZE+1]++;*/
	
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

int createChild(int cid, void (*fptr)(int, int))
{
	int delay = rand()%(5000000) + 1000000;
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
	//printf("%d %d %d \n", rand(), rand(), rand());

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
	
	// shmget - создает новый разделяемый сегмент.
	int shmid = shmget(IPC_PRIVATE, BUFFER_SIZE+2, IPC_CREAT | perms);
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
	buffer[BUFFER_SIZE+1] = 0; //Чтобы считывать буквы по порядку
		
	if (!printBuf())
	{
		perror("buffer == NULL");
		return 1;
	}
	
	//Саздаём производителей и потребителей
	for (int i = 0; i < 3; i++)
	{
		createChild(i+1, &producer);
		createChild(i+1, &consumer);	
	}
	
	int status;
	
	for (int i = 0; i < 3*2; i++)
	{
		wait(&status);
		if (!printBuf())
		{
			perror("buffer == NULL");
			return 1;
		}
	}
	
	/*strcpy(buffer, "Hello");
	if (shmdt(buffer) == -1)
		perror("shmdt");
	return 0;*/

    /*char *buffer;
	if ((buffer = shmat(shmid, NULL, 0)) == (void *)-1)
	{
        perror("shmat error\n");
        return EXIT_FAILURE;
    }*/
	
	
	/*int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    
	
	
	// shmget - создает новый разделяемый сегмент.
	int shmid = shmget(IPC_PRIVATE, shm_size, perms);
	if (shmid == -1)
	{
		perror("shmget error\n");
        return 1;
	}
	
	// Функция shmat() возвращает указатель на сегмент
	// shmbuffer (второй аргумент) равно NULL,
	// то система выбирает подходящий (неиспользуемый)
	// адрес для подключения сегмента
	int *adrs = shmat(shmid, NULL, 0);
	if (*(char *)adrs == -1)
	{
		perror("shmat error");
		return 1;
	}
	
	// В начале разделяемой памяти хранится
	// producer_pos и consumer_pos
	producer_pos = adrs;
	(*producer_pos) = 0;
	consumer_pos = adrs + sizeof(int);
	(*consumer_pos) = 0;
	// Начиная с buffer уже хранятся данные
	buffer = (char *)(adrs + 2 * sizeof(int));
	
	//Заполним буффер нулями
	for (int i = 0; i < N; i++)
		buffer[i] = '0';
	
	//Создаем набор из 3 семафоров
	int fd = semget(IPC_PRIVATE, 3, IPC_CREAT|perms);
    if (fd == -1) //Набор создать не удалось
    {
        perror("semget error\n");
        return 1;
    }
    //Если удачно, то выполняем semop() присваиваем начальные значения
    if (semop(fd, init, 3) == -1) //передаём
	{
		perror("semop error\n");
        return 1;
	}*/
	

	printf("ended\n");
    return 0;
}

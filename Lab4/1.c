// Процессы-сироты.
// В программе создаются не менее двух потомков.
// В потомках вызывается sleep().
// Чтобы предок гарантированно завершился раньше своих помков.
// Продемонстрировать с помощью соответствующего вывода информацию
// об идентификаторах процессов и их группе.

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
	int childpid_1, childpid_2;

	if ((childpid_1 = fork()) == -1)
	{
		perror("Can\'t fork.\n");
		return EXIT_FAILURE;
	}
	else if (childpid_1 == 0)
	{
		sleep(1); // закончится раньше предка
		printf("First child: id: %d ppid: %d  pgrp: %d\n", getpid(), getppid(), getpgrp());
		exit(EXIT_SUCCESS);
	}

	if ((childpid_2 = fork()) == -1)
	{
		perror("Can\'t fork.\n");
		return EXIT_FAILURE;
	}
	else if (childpid_2 == 0)
	{
		sleep(3); // закончится позже предка
		printf("\nSecond child: id: %d ppid: %d  pgrp: %d\n", getpid(), getppid(), getpgrp());
		exit(EXIT_SUCCESS);
	}

	printf("Parent: id: %d pgrp: %d child1: %d child2: %d\n", getpid(), getpgrp(), childpid_1, childpid_2);

	sleep(2);
	return EXIT_SUCCESS;
}
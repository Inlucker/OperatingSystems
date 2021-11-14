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
		sleep(2);
		printf("\nFirst child: pid = %d; ppid = %d;  pgrp = %d\n", getpid(), getppid(), getpgrp());
		exit(EXIT_SUCCESS);
	}

	if ((childpid_2 = fork()) == -1)
	{
		perror("Can\'t fork.\n");
		return EXIT_FAILURE;
	}
	else if (childpid_2 == 0)
	{
		sleep(3);
		printf("Second child: pid = %d; ppid = %d;  pgrp = %d\n", getpid(), getppid(), getpgrp());
		exit(EXIT_SUCCESS);
	}

	printf("Parent: pid = %d; pgrp = %d; child1 = %d; child2 = %d\n", getpid(), getpgrp(), childpid_1, childpid_2);
	
	printf("Parent will die now.\n");
	return EXIT_SUCCESS;
}
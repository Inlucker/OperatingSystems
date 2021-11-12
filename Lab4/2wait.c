#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/wait.h>

void check_status(int status);

int main()
{
	int childpid_1, childpid_2;

	if ((childpid_1 = fork()) == -1)
	{
		perror("Can\'t fork.\n");
		return EXIT_FAILURE;
	}
	else if (!childpid_1)
	{
		sleep(1);
		printf("First child: id: %d ppid: %d  pgrp: %d\n", getpid(), getppid(), getpgrp());
		exit(EXIT_SUCCESS);
	}

	if ((childpid_2 = fork()) == -1)
	{
		perror("Can\'t fork.\n");
		return EXIT_FAILURE;
	}
	else if (!childpid_2)
	{
		sleep(2);
		printf("Second child: id: %d ppid: %d  pgrp: %d\n", getpid(), getppid(), getpgrp());
		exit(EXIT_SUCCESS);
	}

	printf("Parent: id: %d pgrp: %d child1: %d child2: %d\n", getpid(), getpgrp(), childpid_1, childpid_2);

	int status;
	pid_t child_pid;

	printf("Waiting...\n");
	child_pid = wait(&status);
	printf("Child with child_pid: %d and status: %d has ended\n", child_pid, status);

	printf("Waiting...\n");
	child_pid = wait(&status);
	printf("Child with child_pid: %d, status: %d has ended\n", child_pid, status);

	printf("Parent will die now.\n");
	return EXIT_SUCCESS;
}
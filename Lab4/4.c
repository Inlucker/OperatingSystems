#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>

#define LEN 50
#define TEXT1 "My name is Proffesional\n"
#define TEXT2 "There is no meaning in this words\n"

void checkStatus(int child_pid, int status);

int main()
{
	int childpid_1, childpid_2;
	int fd[2];
	// 0 - выход для чтения.
	// 1 - выход для записи.

	if (pipe(fd) == -1)
	{
		perror("Can\'t pipe.\n");
		return EXIT_FAILURE;
	}

	if ((childpid_1 = fork()) == -1)
	{
		perror("Can\'t fork.\n");
		return EXIT_FAILURE;
	}
	else if (!childpid_1) // Это процесс потомок.
	{
		close(fd[0]);
		write(fd[1], TEXT1, strlen(TEXT1) + 1);
		exit(EXIT_SUCCESS);
	}

	if ((childpid_2 = fork()) == -1)
	{
		perror("Can\'t fork.\n");
		return EXIT_FAILURE;
	}
	else if (!childpid_2)
	{
		close(fd[0]);
		write(fd[1], TEXT2, strlen(TEXT2) + 1);
		exit(EXIT_SUCCESS);
	}

	printf("Parent: pid = %d; pgrp = %d; child1 = %d; child2 = %d\n", getpid(), getpgrp(), childpid_1, childpid_2);
	
	char text1[LEN], text2[LEN];

	close(fd[1]);
	read(fd[0], text1, LEN);
	read(fd[0], text2, LEN);

	printf("Text1: %s", text1);
	printf("Text2: %s", text2);

	int status;
	pid_t child_pid;

	printf("Waiting...\n");
	child_pid = wait(&status);
	checkStatus(child_pid, status);

	printf("Waiting...\n");
	child_pid = wait(&status);
	checkStatus(child_pid, status);

	printf("Parent will die now.\n");
	return EXIT_SUCCESS;
}

void checkStatus(int child_pid, int status)
{
	if (WIFEXITED(status))
		printf("Child with pid = %d has terminated normally.\n\n", child_pid);
	else if (WEXITSTATUS(status))
		printf("Child with pid = %d has terminated with code %d.\n", child_pid, WIFEXITED(status));
	else if (WIFSIGNALED(status))
	{
		printf("Child with pid = %d has terminated with an un-intercepted signal.\n", child_pid);
		printf("Signal number = %d.\n", WTERMSIG(status));
	}
	else if (WIFSTOPPED(status))
	{
		printf("Child with pid = %d has stopped.\n", child_pid);
		printf("Signal number = %d.", WSTOPSIG(status));
	}
}
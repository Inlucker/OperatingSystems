#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#define LEN 60
#define TEXT1 "My name is Proffesional\n"
#define TEXT2 "There is no meaning in this words\n"

void checkStatus(int child_pid, int status);

int flag = 0;

void catch_sig(int sig_numb)
{
	flag = 1;
}

int main()
{
	signal(SIGINT, catch_sig);
	
	int childpid_1, childpid_2;
	int fd[2];

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
	else if (childpid_1 == 0)
	{
		sleep(2);
		if (flag)
		{
			close(fd[0]);
			write(fd[1], TEXT1, strlen(TEXT1));
			printf("Child1 has sent the meassge\n");
		}
		else
			printf("Child1 hasn't sent the message\n");
		exit(EXIT_SUCCESS);
	}

	if ((childpid_2 = fork()) == -1)
	{
		perror("Can\'t fork.\n");
		return 1;
	}
	else if (childpid_2 == 0)
	{
		sleep(2);
		if (flag)
		{
			close(fd[0]);
			write(fd[1], TEXT2, strlen(TEXT2));
		}
		else
			printf("Child2 hasn't sent the message\n");
			exit(EXIT_SUCCESS);
	}

	printf("Parent: pid = %d; pgrp = %d; child1 = %d; child2 = %d\n", getpid(), getpgrp(), childpid_1, childpid_2);
	printf("Press \"CTRL+C\", if you want childs to send messages.\n");
	printf("In other case they will not.\n\n");

	int status;
	pid_t child_pid;

	printf("Waiting...\n");
	child_pid = wait(&status);
	checkStatus(child_pid, status);

	printf("Waiting...\n");
	child_pid = wait(&status);
	checkStatus(child_pid, status);
	
	if (flag)
	{
		char text[LEN] = "";
		
		close(fd[1]);
		read(fd[0], text, LEN);
		printf("Received message: %s\n", text);
	}
	
	else
	{
		printf("No signal have been sent\n");
	}

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
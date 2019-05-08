/*
Main Shell

Shaydon Bodemar
Chadmond Wu
05/7/19
*/

#include "command_line.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdbool.h>
#include <signal.h>
#define MAX_LINE_LENGTH 512

pid_t curr;


void sig_handler(int sig) {
    int tmp_errno = errno;
    pid_t pid = getpid();
    if(curr != pid){
        printf("Terminated %d with %d", pid, sig);
        exit(1);
    }
    errno = tmp_errno;
}

void sigchld_handler(int sig) 
{
	int tmp_errno = errno;
	if (sig == SIGCHLD)
	{
		int state;
		pid_t pid;
		while ((pid = waitpid(-1, &state, WNOHANG)) > 0)
		{
			if (WIFEXITED(state))
				printf("Child exited with %d\n", WEXITSTATUS(state));
			else if (WTERMSIG(state))
				printf("Child exited with signal %d\n", WTERMSIG(state));
		}
	}
    errno = tmp_errno;
}

int builtin_command(char **argv)
{
	if (!strcmp(argv[0], "quit"))
		exit(0);
	if (!strcmp(argv[0], "&"))
		return 1;
    if(!strcmp(argv[0], "cd"))
	{
        if(chdir(argv[1]) != 0)
		{
            perror("cd failed, did not change directory");
            return 1;
        }
    }
    return 0;
}

void eval(char **argv, bool background)
{
    pid_t pid;
    int status;
    if(argv[0] == NULL)
        return;

    if(!builtin_command(argv))
	{
        if((pid = fork()) == 0)
		{
            if(execvp(argv[0], argv) < 0)
			{
                printf("%s is an invalid command. \n", argv[0]);
                exit(0);
            }
        } 
		else if(pid < 0)
            perror("invalid error"); 
		else
		{
			if(!background)
				waitpid(pid, &status, WUNTRACED);
        }
    }
}

int main(int argc, const char **argv)
{
    signal(SIGCHLD, sigchld_handler);
    curr = getpid();
    struct sigaction sigact;
    sigact.sa_handler = &sigchld_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESTART;

    if(sigaction(SIGCHLD, &sigact, 0) == -1)
	{
        perror(0);
        exit(1);
    }

    signal(SIGINT, sig_handler);

    char cmdline[MAX_LINE_LENGTH];
    struct CommandLine command;
    for (;;)
    {
        printf("> ");
        fgets(cmdline, MAX_LINE_LENGTH, stdin);
        if (feof(stdin)) 
            exit(0);

        bool gotLine = parseLine(&command, cmdline);
        if (gotLine) 
        {
            eval(command.arguments, command.background);
            printCommand(&command);
            freeCommand(&command);
        }
		memset(cmdline, 0, sizeof(cmdline));
    }
}

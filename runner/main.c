// "shell" for running c programs from this directory
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define PATH "./apps/"


int child_pid = 0;

void run(const char* app){
    char path[64] = { PATH };
    strcat(path, app);
    child_pid = fork();
    if(child_pid < 0){
	printf("Error! Coulnd't create a process!\n");
	exit(1);
    }else if(child_pid == 0){
	char* args[] = {NULL};
	printf("=======================\n");
	execve(path, args, NULL);
	printf("ERRORORRORR!!!!!!\n");
	exit(-1);
    }else{
	int status;
	waitpid(child_pid, &status, WUNTRACED);
	printf("=======================\n");
	printf("The program finished running with return code: %d\n", WEXITSTATUS(status));
	child_pid = 0;
    }
}

void kill_child(){
    if(child_pid == 0){
	printf("\nClosing runnner shell...\n");
	exit(0);
    }
    child_pid = 0;
}

int main(int argc, char *argv[])
{
    // handle ctrl-c
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = kill_child;
    sigaction(SIGINT, &action, NULL);

    char buff[64];
    printf("Welcome to the runner shell!\n");
    // main loop
    while(1){
	printf("&");
	scanf("%s", buff);
	run(buff);
    } 
    return 0;
}

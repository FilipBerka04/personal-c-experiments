// "shell" for running c programs from this directory
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


void run(const char* path){
    int rc = fork();
    if(rc < 0){
	printf("Error! Coulnd't create a process!\n");
	exit(1);
    }else if(rc == 0){
	char* args[] = {NULL};
	execve(path, args, NULL);
    }else{
	int status;
	wait(&status);
	printf("=======================\n");
	printf("The program finished running with return code: %d\n", WEXITSTATUS(status));
    }
}

int main(int argc, char *argv[])
{
    printf("Welcome to the runner shell!\n");
    // main loop
    while(1){
	printf("Press Enter to run the test script");
	getchar();
	printf("=======================\n");
	run("./hello/builds/hello");
    } 
    return 0;
}

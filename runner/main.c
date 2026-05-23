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
    char path[64] = PATH;
    char* child_argv[32] = { NULL };
    strcat(path, app);
    //TODO
    int i = 0;
    int count = 0;
    do{
	child_argv[count++] = &(path[i++]);
	//skip the rest of the word
	while(path[i] >= 33 && path[i] <=126)i++;
	if(path[i] == ' ') path[i++] = 0;
	//skip the rest of spaces
	while(path[i]==' ')i++;
    }while(path[i] != '\0');

    child_pid = fork();
    int status;
    if(child_pid < 0){
	// printf("Error! Coulnd't create a process!\n");
	exit(1);
    }else if(child_pid == 0){
	printf("=======================\n");
	execve(child_argv[0], child_argv, NULL);
	// Exec failed
	exit(127);
    }else{
	waitpid(child_pid, &status, WUNTRACED);
	int code;
	printf("=======================\n");
	//command usage error
	if(WEXITSTATUS(status) == 127)
	    printf("Error executing %s, command not found!\n", path);
	//exited normally
	if(WIFEXITED(status))
	    code = WEXITSTATUS(status);
	//killed by a signal
	else if(WIFSIGNALED(status))
	    code = WTERMSIG(status) + 128;
	printf("The program finished running with return code: %d\n", code);
	child_pid = 0;
	
    }
}

void kill_child(){
    if(child_pid == 0){
	printf("\nClosing runnner shell...\n");
	exit(0);
    }
}

int main(int argc, char *argv[]){
    // handle ctrl-c
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = kill_child;
    // line bolow makes it so after the action exetutes, all previous system calls shall be restarted
    // required, since wait() inside of run() needs to capture the zombie child will get interrupted by the signal
    action.sa_flags = SA_RESTART;
    sigaction(SIGINT, &action, NULL);

    char buff[64];
    printf("Welcome to the runner shell!\n");
    // main loop
    while(1){
	printf("&");
	fgets(buff, 64, stdin);
	if(*buff != '\n'){
	    buff[strcspn(buff, "\n")] = 0;
	    run(buff);
	}
    } 
    return 0;
}

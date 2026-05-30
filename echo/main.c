#include <stdio.h>

#define MAX_LENGTH 256
int main(int argc, char *argv[])
{
    printf("Argc: %d\n", argc);
    for(int i = 1; i < argc; i++)
	printf("%s\n", argv[i]);
    return 0;
}

#include <stdio.h>

#define MAX_LENGTH 256

int main(int argc, char *argv[])
{
    char buff[MAX_LENGTH];
    fgets(buff, MAX_LENGTH, stdin);
    fputs(buff, stdout);
    return 0;
}

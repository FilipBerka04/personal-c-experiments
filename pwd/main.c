#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    int max = 10;
    DIR* dirPtr;
    struct dirent* dp;
    ino_t ino;
    char path[256];
    char buff[256];
    do {
        int diff = 0, found = 0;
        struct stat parent_st, child_st, current_st;
        lstat("..", &parent_st);
        lstat(".", &current_st);
        if (!max--)
            return 0;
        // open the current dir and save its id
        dirPtr = opendir(".");
        // get the serial number of the current directory
        while ((dp = readdir(dirPtr)) != NULL) {
            if (!strcmp(dp->d_name, ".")) {
                ino = dp->d_ino;
                break;
            }
        }
        if (parent_st.st_ino != current_st.st_ino) {
            ino = current_st.st_ino;
            diff = 1;
        }
        closedir(dirPtr);
        // go to the parent dir
        chdir("..");
        // find the name of the dir we went out of
        dirPtr = opendir(".");
        // find the directory we went out of
        while ((dp = readdir(dirPtr)) != NULL) {
            if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
                continue;
            lstat(dp->d_name, &child_st);
            if (!diff && dp->d_ino == ino) {
		found = 1;
                //printf("Moved out of %s, %lu\n", dp->d_name, dp->d_ino);
                strcpy(buff, "/");
                strcat(buff, dp->d_name);
                strcat(buff, path);
                strcpy(path, buff);
                break;
            } else if (diff && current_st.st_ino == child_st.st_ino) {
		found = 1;
                //printf("Moved out of %s, %lu\n", dp->d_name, dp->d_ino);
                strcpy(buff, "/");
                strcat(buff, dp->d_name);
                strcat(buff, path);
                strcpy(path, buff);
                break;
            }
        }
        if (!found) {
            printf("%s\n", path);
            closedir(dirPtr);
            return 0;
        }
    } while (1);

    return 1;
}

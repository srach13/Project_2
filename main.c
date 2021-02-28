#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

extern char **environ;
char error_message[30] = "An error has occurred\n";

int main() {
    printf("Hello, World!\n");
    return 0;
}

int cd_command(char **args) {
    char curr_dir[1024];    //initialize variable to store name of current directory
    getcwd(curr_dir,sizeof(curr_dir));  //get current directory
    if(args[1] == NULL) {   //print current directory if no other arguments
        printf("current directory: %s\n", curr_dir);
    }
    else {  //handles cd error
        if(chdir(args[1]) != 0) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }
    return 0;
}

int clr_command() {
    printf("\033[H\033[2J");    //ANSI terminal control escape sequences
}

int dir_command(char **args) {
    DIR *directory;
    struct dirent *reader;
    directory = opendir("./");  //opens current directory
    if (directory != NULL) {
        while ((reader = readdir(directory)))   //reads through contents in current directory
            puts(reader->d_name);   //prints directory
        (void) closedir(directory); //closes directory
    } else {    //handles displaying directory error
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
    return 0;
}

int environ_command() {
    for (int i = 0; environ[i] != NULL; i++)    //iterate through & print out all the environ strings
        printf("%s\n", environ[i]);
}


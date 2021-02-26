#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
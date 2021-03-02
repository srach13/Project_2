#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <fcntl.h>

#define ARGUMENT_SIZE 1024

extern char **environ;
char error_message[30] = "An error has occurred\n";

int main() {
    printf("Hello, World!\n");
    return 0;
}

int launch_process(char **args) {
    pid_t pid, wpid;
    int status;
    int flag = false;
    char processName[strlen(args[0])];
    memset(processName, '\0', sizeof(processName));
    if(args[0][strlen(args[0])-1] == '&') {
        strncpy(processName, args[0], strlen(args[0]) - 1);
        flag = true;
    } else {
        strcpy(processName,args[0]);
    }
    if((pid = Fork()) == 0) {
        if (execvp(processName, args) < 0) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(EXIT_FAILURE);
        }
        exit(-1);
    } else {
        if(!flag) {
            do {
                wpid = waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }
    return 1;
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
    return 0;
}

int echo_command(char **args) {
    int i = 1;
    while(args[i] != NULL) {    //iterate through array strings and print
        printf("%s ", args[i]);
        i++;    //increment counter variable
    }
    printf("%s", "\n"); //separate next command prompt with new line character
}

int help_command() {
    char *manual[] = {"more", "readme", NULL};
    return launch_process(manual);
}

int pause_command() {
    printf("Shell has been paused. Press 'Enter' to continue");
    char c = getchar();
    while(c != '\n') {  //wait for user to press enter
        c = getchar();
    }
    return 0;
}

int quit(char *command) {
    if(strcmp(command, "quit") == 0) {
        return 0;
    } else if(strcmp(command, "QUIT") == 0) {
        return 0;
    } else {
        return 1;
    }
}

int shell_environment(char **args) {
    if(args[1] == NULL) {
        char *environment[] = {"environment", NULL};
        return launch_process(environment);
    } else {
        if(putenv(args[1]) != 0) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }
    return 0;
}

pid_t Fork(void) {
    pid_t pid;
    if((pid = fork()) < 0) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(EXIT_FAILURE);
    }
    return pid;
}
void redirect(char* argv[ARGUMENT_SIZE], int flag, int i) {
    int j = i; //cycles through input starting from where the token of interest is
    int in, out; //will hold saved states of stdin and stdout
    pid_t pid; //forks process
    int savedin = dup(0); //saves the read side
    int savedout = dup(1); //saves the write side

    pid = fork(); //forks process into parent and child

    if(pid < 0) //error
    {
        printf("%s", "Error, can't fork\n"); //handles error
    }

    else if(pid == 0)//child process
    {
        if(flag == 1) // > is the redirect token
        {
            int output = open(argv[j + 1], O_WRONLY|O_CREAT,S_IRWXU|S_IRWXG|S_IRWXO); //opens the file for writing to or creating
            dup2(output, 1); //dups write side
            close(output); //closes stdout
            argv[j] = NULL; //sets > to null
            argv[j+1] = NULL; //sets file to null
            ++j; //increments
        }

        if(flag == 2) // < is the redirect token
        {
            int input = open(argv[j + 1], O_CREAT | O_RDONLY, 0666); //opens the file for reading
            dup2(input, 0); //dups read so it's from the input file
            close(input); //closes stdin
            argv[j] = NULL; //sets < to null
            argv[j+1] = NULL; //sets file name to null
            ++j; //increment counter
        }

        if(flag == 3) // >> is the redirect token
        {
            int output = open(argv[j + 1], O_WRONLY | O_APPEND | O_CREAT, 0666); //opens the file to be appended to
            dup2(output, 1); //dups writing
            close(output); //closes stdin
            argv[j] = NULL; //sets >> to null
            argv[j+1] = NULL; //sets file name to null
            ++j; //increments counter
        }
        execvp(argv[0], argv); //executes given commands
    }

    else if(pid > 0)//parent process
    {
        waitpid(pid, NULL, WCONTINUED); //waits for child process to finisg
    }

    dup2(savedin, 0); //restores read
    close(savedin); //closes savedin
    dup2(savedout, 1); //restores stdout
    close(savedout); //closes out
}

int check_redirect(char* argv[ARGUMENT_SIZE], int isBackground) {
    int redirected = 0; //flag set to true if there is an I/O token
    char* leftSide[ARGUMENT_SIZE]; //holds left side of the token
    char* rightSide[ARGUMENT_SIZE]; //holds the right side of the token
    int i = 0; //checks entire array for arrows
    while(argv[i] != NULL) //while there are strings left to check
    {
        if(ispipe(argv, leftSide, rightSide) == 1) //checks to see if there is a pipe in the array
        {
            pipeEvaluate(leftSide, rightSide, isBackground); //calls pipe function to handle the rest
            return 2; //returns pipe flag
        }

        if(strcmp(argv[i], ">") == 0) //if the found token is >
        {
            redirect(argv, 1, i); //redirects with the flag set to 1
            redirected = 1; //sets redirect flag to true
        }

        if(strcmp(argv[i], "<") == 0)
        {
            redirect(argv, 2, i); //redirects with the flag set to 2
            redirected = 1; //sets redirect flag to true
        }

        if(strcmp(argv[i], ">>") == 0)
        {
            redirect(argv, 3, i); //redirects with the flag set to 3
            redirected = 1; //sets redirect flag to true
        }

        i++; //increments counter

    }
    return redirected; //returns flag

}


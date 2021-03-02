#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <fcntl.h>

#define ARGUMENT_SIZE 1024  //size of input buffer
#define COMMAND_SIZE 7  //internal commands

extern char **environ;
char error_message[30] = "An error has occurred\n";
int length; //keeps track of actual size of input since input buffer is 1024

void parseLine(char *line, char* argv[ARGUMENT_SIZE]);  //parses input into string tokens
int launch_process(char **args);
int cd_command(char **args);
int clr_command();
int dir_command(char **args);
int environ_command();
int echo_command(char **args);
int help_command();
int pause_command();
int quit(char *command);
int shell_environment(char **args);
pid_t Fork(void);   //forks process
void redirect(char* argv[ARGUMENT_SIZE], int flag, int i);  //redirects input
int check_redirect(char* argv[ARGUMENT_SIZE], int isBackground);    //checks whether the input needs to be redirected
int check_pipe(char* argv[ARGUMENT_SIZE], char* left[ARGUMENT_SIZE], char* right[ARGUMENT_SIZE]);   //check whether the input has a pipe
void run_pipe(char* left[ARGUMENT_SIZE], char* right[ARGUMENT_SIZE], int isBackground); //runs the piping proccess
int check_internal(char* argv[]);
int run_internal_command(char* argv[ARGUMENT_SIZE]);

int main() {
    printf("Hello, World!\n");
    return 0;
}

void parseLine(char *line, char* argv[ARGUMENT_SIZE]) {
    char* token; //initializes token to save string
    const char t[] = {" \n"}; //separates the raw input
    int counter = 0; //initializes counter

    token = strtok(line, t); //saves the first word
    argv[counter] = token; //stores the first word into the array
    counter++; //increments counter

    while (token != NULL) {     //while there are still strings left in the input
        token = strtok(NULL, t); //continues taking more strings
        argv[counter] = token; //continues saving into array
        counter++; //continues incrementing
    }
    length = counter - 1; //keeps track of the actual length of the array
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

char* internal_commands[] = //holds all the internal commands
        {
                "cd",
                "clr",
                "dir",
                "environ",
                "echo",
                "help",
                "pause",
        };

int (*internal_functions[])(char **) = //holds addresses to the functions
        {
                &cd_command,
                &clr_command,
                &dir_command,
                &environ_command,
                &echo_command,
                &help_command,
                &pause_command,
        };

int check_internal(char* argv[]) {
    int i;
    //iterate through internal commands and see if the input matches an internal command
    for (i = 0; i < sizeof(internal_commands) / sizeof(internal_commands[0]); i++) {
        if(strcmp(argv[0], internal_commands[i]) == 0) {
            return 1; //true
        }
    }
    return 0; //false
}

int run_internal_command(char* argv[ARGUMENT_SIZE]) {
    int i;
    for(i = 0; i < COMMAND_SIZE; i++) //cycles through list of built in functions
    {
        if(strcmp(argv[0], internal_commands[i]) == 0) //if there is a match
        {
            return (*internal_functions[i])(argv); //runs built in
        }
    }
    return 0; //function finished
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
    char* left[ARGUMENT_SIZE]; //holds left side of the token
    char* right[ARGUMENT_SIZE]; //holds the right side of the token
    int i = 0; //checks entire array for arrows
    while(argv[i] != NULL) //while there are strings left to check
    {
        if(check_pipe(argv, left, right) == 1) //checks to see if there is a pipe in the array
        {
            run_pipe(left, right, isBackground); //calls pipe function to handle the rest
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

int check_pipe(char* argv[ARGUMENT_SIZE], char* left[ARGUMENT_SIZE], char* right[ARGUMENT_SIZE]) {
    int pipeNum = 0; //flag to test if pipe is in it

    int i = 0; //counter to cycle through array
    int j, k; //cycles through left and right side to save strings into it
    while(argv[i] != NULL) //while array still has strings
    {
        if (strcmp(argv[i], "|") == 0) //if the pipe is there!
        {
            for (j = 0; j < i; j++) //cycles through strings on the left side of the pipe
            {
                left[j] = malloc(sizeof(char) * sizeof(argv[i])); //allocates memory for specified array element
                char *strArg = argv[j]; //sets token into string
                left[j] = strArg; //stores pointer into array
            }

            int m = 0; //specifically for right side array elements
            for (k = i + 1; k < length; k++) //while we haven't reached the end of the array
            {
                right[m] = malloc(sizeof(char) * sizeof(argv[k])); //allocates memory for specified array element
                char *strArg = argv[k]; //sets pointer to string
                right[m] = strArg; //puts pointer in the array
                m++; //increments right side array counter
            }
            pipeNum = 1; //sets pipe flag to true
        }
        i++; //increments original array counter
    }
    return pipeNum; //returns flag
}

/*Called by ispipe, this function handles everything that is needed to successfully
  run the pipe command. It takes in the left and right sides of the original array
  and a background flag.*/
void run_pipe(char* left[ARGUMENT_SIZE], char* right[ARGUMENT_SIZE], int isBackground)
{
    int fd[2]; //read and write end of the pipe
    pid_t pid; //for the first child process
    pid_t pid2; //for the second child process
    int savedin = dup(0); //saves state of read
    int savedout = dup(1); //saves state of write

    pipe(fd); //pipes read and write end
    pid = fork(); //forks process

    if(pid < 0) //error in forking
    {
        perror("Error forking\n"); //handles forking error
        exit(1);
    }

    if(pid == 0) //child process 1
    {
        dup2(fd[1], STDOUT_FILENO); //dups standard output
        close(fd[0]); //closes read side

        if(check_internal(left) == 1) //checks for built in all on its own
        {
            run_internal_command(left); //runs built in right into here
        }

        else //not a built in
        {
            if(execvp(left[0], left) < 0) //checks for error in executing
            {
                printf("%s", "Error exec\n"); //handles error
            }
        }
    }

    else //parent process
    {
        pid2 = fork(); //forks again into child process 2

        if(pid2 < 0) //error
        {
            perror("Error forking\n"); //handles error
            exit(1);
        }

        else if (pid2 == 0) //child process 2
        {
            dup2(fd[0], STDIN_FILENO); //dups read end
            close(fd[1]); //closes write end

            if(check_internal(right) == 1) //checks for built in once again
            {
                run_internal_command(right); //runs built in...once again
            }
            else //not a built in, my friend
            {
                if(execvp(right[0], right) < 0) // checks for executing errors
                {
                    printf("%s", "Error exec\n"); //execution handled ;)
                }
            }
        }

        else if(pid2 > 0 && isBackground == 0)//parent process yet again...this time with NO background!
        {
            close(fd[0]); //closes read
            close(fd[1]); //closes write
            waitpid(pid, NULL, 0); //waits for first child process to finish
            waitpid(pid2, NULL, 0); //waits for second child process to finish.
        }
    }
} //end of file


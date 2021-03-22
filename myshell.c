//HEADERS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

//DECLARATIONS
#define ARGUMENT_SIZE 1024 //size of input buffer
#define COMMAND_SIZE 7 //internal commands

//GLOBAL VARIABLES
extern char **environ; //environ variable
char error_message[30] = "An error has occurred\n"; //error message
int length; //keeps track of actual size of input

//FUNCTIONS FOR HANDLING INPUT & USER COMMANDS
void print_cwd();   //prints current working directory
char *read_input();   //reads input
void parse_input(char *line, char* argv[ARGUMENT_SIZE]); //parses input into string tokens
int launch_process(char *line); //launches process

//FUNCTIONS FOR BUILT IN COMMANDS
int cd_command(char **args); //changes directory
int clr_command(); //clears screen
int dir_command(char **args); //lists contents of directory
int environ_command(); //lists all the environment strings
int echo_command(char **args); //displays comment
int help_command(); //displays user manual
int pause_command(); //pauses shell
int quit(char *command); //quits shell
int shell_environment(char **args); //contains full path for shell executable

//FUNCTIONS FOR FORKING, REDIRECTING, PIPING
void forkProgram(char* argv[ARGUMENT_SIZE], int isBackground); //forks program
void redirect(char* argv[ARGUMENT_SIZE], int flag, int i); //redirects input
int check_redirect(char* argv[ARGUMENT_SIZE], int isBackground); //checks whether the input needs to be redirected
int check_pipe(char* argv[ARGUMENT_SIZE], char* left[ARGUMENT_SIZE], char* right[ARGUMENT_SIZE]); //checks whether the input has a pipe
void run_pipe(char* left[ARGUMENT_SIZE], char* right[ARGUMENT_SIZE], int isBackground); //runs the piping process
int check_internal(char* argv[]); //checks whether the input has an internal command
int run_internal_command(char* argv[ARGUMENT_SIZE]); //runs the internal commands

/*
 *
 */

//MAIN FUNCTION THAT WILL RUN WHILE LOOP TO TAKE & HANDLE COMMANDS
int main(int argc, char* argv[]) {

    if (argc > 1) { //command is for batch file

        char *cmd_file[] = {"batch.txt"};
        char input[ARGUMENT_SIZE];
        char *line;

        FILE *fp = fopen(cmd_file[0], "r"); //open batch file to read

        while (fgets(input, sizeof(input), fp) != NULL) {   //read each line from file
            line = input;   //set input to line read
            launch_process(line);   //pass line to launch_process function
        }

        fclose(fp);
    }

    else {  //read command from user

        char *line_to_read; //user input
        system("clear");

        while (1) { //endless loop
            print_cwd();    //call function to print cwd
            line_to_read = read_input(); //read input from user

            if (strcmp(line_to_read, "quit\n") == 0 || feof(stdin)) {   //restrict while loop
                return EXIT_SUCCESS;
            }

            launch_process(line_to_read);   //pass input to launch_process function
        }
    }
}

//PRINT CURRENT WORKING DIRECTORY
void print_cwd() {

    char host[ARGUMENT_SIZE]; //hold host name
    char cwd[1024]; //hold cwd

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        gethostname(host, ARGUMENT_SIZE); //save host name to array
        printf("%s%smyshell>", host, cwd); //print host name & cwd
    }

    else {  //handle error
        write(STDERR_FILENO, error_message, strlen(error_message));
    }

}

//READ INPUT
char *read_input() {

    char input[ARGUMENT_SIZE]; //hold input
    char *line; //read input

    fgets(input, sizeof(input), stdin);
    line = input; //save input to line variable

    return line;

}

//PARSE INPUT INTO STRING TOKENS
void parse_input(char *line, char* argv[ARGUMENT_SIZE]) {

    char* token;
    const char t[] = {" \n"}; //separate the input
    int counter = 0;

    token = strtok(line, t); //save the first word into token
    argv[counter] = token; //store the first word into array
    counter++; //increment counter

    while (token != NULL) { //while there are still strings left to parse from the input
        token = strtok(NULL, t); //take next string
        argv[counter] = token; //save into array
        counter++; //increment
    }

    length = counter - 1; //keep track of the actual length of the array
}

//LAUNCH EACH COMMAND'S PROCESS
int launch_process(char *line) {

    char *argv[ARGUMENT_SIZE]; //hold input
    int isBackground = 0; //check if input has a & to see if it needs to be ran in background

    parse_input(line, argv); //split input into strings

    if (strcmp(argv[length - 1], "&") == 0) { //check if input array has a &
        isBackground = 1; //set to true
        argv[length - 1] = NULL; //delete the & from the input
    }

    int t = check_redirect(argv, isBackground); //call check_redirect function

    if (t == 2)  { //if check_redirect token, return pipe flag
        return 1;
    }

    if (t == 0)  { //if no redirect tokens

        if (argv[0] == NULL) { //if no inputs
            return 0;
        }

        if (check_internal(argv) == 1)  { //if internal command
            run_internal_command(argv); //call function to run internal command

        } else { //exec as fork
            fork();
            return 1;
        }

        return 1;
    }

    return 0;
}

/*
 *
 */

//CHANGE DIRECTORY
int cd_command(char **args) {

    char curr_dir[1024];
    getcwd(curr_dir,sizeof(curr_dir)); //get current directory

    if (args[1] == NULL) { //print current directory if there are no other arguments
        printf("current directory: %s\n", curr_dir);
    }

    else { //handle cd error
        if (chdir(args[1]) != 0) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }

    return 0;
}

//CLEAR THE DISPLAY
int clr_command() {
    printf("\033[H\033[2J"); //ANSI terminal control escape sequences
    return 0;
}

//LIST CONTENTS OF DIRECTORY
int dir_command(char **args) {

    DIR *directory;
    struct dirent *reader;
    directory = opendir("./"); //open current directory

    if (directory != NULL) {
        while ((reader = readdir(directory))) //read through contents in current directory
            puts(reader->d_name); //print directory
        (void) closedir(directory); //close directory

    } else { //handle displaying directory error
        write(STDERR_FILENO, error_message, strlen(error_message));
    }

    return 0;
}

//LIST ENVIRONMENT STRINGS
int environ_command() {
    for (int i = 0; environ[i] != NULL; i++) //iterate through & print out all the environment strings
        printf("%s\n", environ[i]);
    return 0;
}

//DISPLAY COMMENT
int echo_command(char **args) {

    int i = 1;

    while (args[i] != NULL) { //iterate and print array strings
        printf("%s ", args[i]);
        i++; //increment counter variable
    }

    printf("%s", "\n"); //separate next command prompt with new line character
    return 0;
}

//DISPLAY USER MANUAL
int help_command() {
    char *manual[] = {"more", "readme", NULL};
    return launch_process((char *) manual);
}

//PAUSE SHELL UNTIL THE USER PRESSES ENTER
int pause_command() {

    printf("Shell has been paused. Press 'Enter' to continue");
    char c = getchar();
    while (c != '\n') { //wait for user to press enter
        c = getchar();
    }

    return 0;
}

//QUIT SHELL
int quit(char *command) {

    if (strcmp(command, "quit") == 0) {
        return 0;

    } else if (strcmp(command, "QUIT") == 0) {
        return 0;

    } else {
        return 1;
    }

}

//FULL PATH FOR SHELL EXECUTABLE
int shell_environment(char **args) {

    if (args[1] == NULL) {
        char *environment[] = {"environment", NULL};
        return launch_process((char *) environment);

    } else {
        if (putenv(args[1]) != 0) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }

    return 0;
}

//HOLD ALL INTERNAL COMMANDS
char* internal_commands[] =
        {
                "cd",
                "clr",
                "dir",
                "environ",
                "echo",
                "help",
                "pause",
        };

//HOLD ADDRESSES TO INTERNAL COMMAND FUNCTIONS
int (*internal_functions[])(char **) =
        {
                &cd_command,
                &clr_command,
                &dir_command,
                &environ_command,
                &echo_command,
                &help_command,
                &pause_command,
        };

//CHECK IF INPUTTED COMMAND IS INTERNAL
int check_internal(char* argv[]) {

    int i;

    for (i = 0; i < sizeof(internal_commands) / sizeof(internal_commands[0]); i++) { //iterate through all the internal commands to see if the input matches one of them
        if (strcmp(argv[0], internal_commands[i]) == 0) {
            return 1; //return true if there is a match
        }
    }

    return 0; //return false otherwise
}

//RUN INTERNAL COMMAND
int run_internal_command(char* argv[ARGUMENT_SIZE]) {

    int i;

    for (i = 0; i < COMMAND_SIZE; i++) { //iterate through all the internal commands
        if (strcmp(argv[0], internal_commands[i]) == 0) { //if there is a match
            return (*internal_functions[i])(argv); //run internal command function
        }
    }

    return 0;
}

/*
 *
 */

//FORK PROCESS
void fork_command(char* argv[ARGUMENT_SIZE], int isBackground) {

    pid_t pid;
    pid = fork();

    if (pid < 0) {  //handle error
        write(STDERR_FILENO, error_message, strlen(error_message));
    }

    else if (pid == 0) { //child process

        if (execvp(argv[0], argv) < 0) {

            write(STDERR_FILENO, error_message, strlen(error_message)); //handle error
            exit(0);

        }

        write(STDERR_FILENO, error_message, strlen(error_message));

    }

    else if (pid > 0 && isBackground == 0) { //parent process with no background

        int status; //wait status for parent

        if (waitpid(pid, &status, 0) < 0) { //handle error
            write(STDERR_FILENO, error_message, strlen(error_message));
        }

    }

}

//REDIRECTION
void redirect(char* argv[ARGUMENT_SIZE], int flag, int i) {

    int j = i; //iterate through input starting from the necessary token
    pid_t pid; //fork process

    int saved_read = dup(0); //save the read side
    int saved_write = dup(1); //save the write side

    pid = fork(); //fork process into parent and child

    if (pid < 0) { //handle forking error
        write(STDERR_FILENO, error_message, strlen(error_message));
    }

    else if (pid == 0) { //child process
        if (flag == 1) { // > is the redirect token
            int output = open(argv[j + 1], O_WRONLY|O_CREAT,S_IRWXU|S_IRWXG|S_IRWXO); //open the file for writing or creating
            dup2(output, 1); //dup write side
            close(output); //close stdout
            argv[j] = NULL; //set > to null
            argv[j+1] = NULL; //set file to null
            ++j; //increment
        }

        if (flag == 2) { // < is the redirect token
            int input = open(argv[j + 1], O_CREAT | O_RDONLY, 0666); //open the file for reading
            dup2(input, 0); //dup read side
            close(input); //close stdin
            argv[j] = NULL; //set < to null
            argv[j+1] = NULL; //set file to null
            ++j; //increment
        }

        if (flag == 3) { // >> is the redirect token
            int output = open(argv[j + 1], O_WRONLY | O_APPEND | O_CREAT, 0666); //open the file for appending
            dup2(output, 1); //dup writing
            close(output); //close stdin
            argv[j] = NULL; //set >> to null
            argv[j+1] = NULL; //set file to null
            ++j; //increment
        }

        execvp(argv[0], argv); //execute given commands
    }

    else if (pid > 0) { //parent process
        waitpid(pid, NULL, WCONTINUED); //wait for child process to finish
    }

    dup2(saved_read, 0); //restore in
    close(saved_read); //close in
    dup2(saved_write, 1); //restore out
    close(saved_write); //close out
}

//CHECK FOR REDIRECTION
int check_redirect(char* argv[ARGUMENT_SIZE], int isBackground) {

    int redirected = 0;
    char* left[ARGUMENT_SIZE]; //hold left side of the token
    char* right[ARGUMENT_SIZE]; //hold right side of the token
    int i = 0;

    while (argv[i] != NULL) {
        if (check_pipe(argv, left, right) == 1) { //check to see if there is a pipe
            run_pipe(left, right, isBackground); //call pipe function
            return 2; //return pipe flag
        }

        if (strcmp(argv[i], ">") == 0) { //if the found token is >
            redirect(argv, 1, i); //redirect with the flag set to 1
            redirected = 1; //redirect flag = true
        }

        if (strcmp(argv[i], "<") == 0) {
            redirect(argv, 2, i); //redirect with the flag set to 2
            redirected = 1; //redirect flag = true
        }

        if (strcmp(argv[i], ">>") == 0) {
            redirect(argv, 3, i); //redirect with the flag set to 3
            redirected = 1; //redirect flag = true
        }

        i++;
    }

    return redirected; //return flag
}

//CHECK FOR PIPES
int check_pipe(char* argv[ARGUMENT_SIZE], char* left[ARGUMENT_SIZE], char* right[ARGUMENT_SIZE]) {

    int pipeNum = 0;
    int i = 0;
    int j, k;

    while (argv[i] != NULL) {
        if (strcmp(argv[i], "|") == 0) { //check if argument contains pipe

            for (j = 0; j < i; j++) { //iterate through strings on the left side of the pipe
                left[j] = malloc(sizeof(char) * sizeof(argv[i])); //allocate memory for element
                char *strArg = argv[j]; //set pointer to string
                left[j] = strArg; //store pointer into array
            }

            int m = 0;

            for (k = i + 1; k < length; k++) { //iterate through strings on the right side of the pipe
                right[m] = malloc(sizeof(char) * sizeof(argv[k])); //allocate memory for element
                char *strArg = argv[k]; //set pointer to string
                right[m] = strArg; //store pointer into array
                m++; //increment counter for right side of array
            }

            pipeNum = 1; //set pipe flag = true
        }

        i++; //increment original array counter
    }

    return pipeNum; //return flag
}

//PIPING
void run_pipe(char* left[ARGUMENT_SIZE], char* right[ARGUMENT_SIZE], int isBackground) {

    int fd[2]; //read and write end of the pipe
    pid_t pid; //child process 1
    pid_t pid2; //child process 2

    pipe(fd); //pipe read and write end
    pid = fork(); //fork process

    if (pid < 0) { //handle forking error
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    if (pid == 0) { //child process 1
        dup2(fd[1], STDOUT_FILENO); //dup standard output
        close(fd[0]); //close read side

        if (check_internal(left) == 1) { //check for internal
            run_internal_command(left); //run internal
        } else { //not internal
            if (execvp(left[0], left) < 0) { //handle executing error
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
        }
    } else { //parent process
        pid2 = fork();
        if (pid2 < 0) { //handle forking error
            write(STDERR_FILENO, error_message, strlen(error_message));
        } else if (pid2 == 0) { //child process 2
            dup2(fd[0], STDIN_FILENO); //dup read end
            close(fd[1]); //close write end

            if (check_internal(right) == 1) { //check for internal
                run_internal_command(right); //run internal
            } else { //not internal
                if (execvp(right[0], right) < 0) { //handle executing error
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
            }
        } else if (pid2 > 0 && isBackground == 0) { //parent process with no background
            close(fd[0]); //close read
            close(fd[1]); //close write
            waitpid(pid, NULL, 0); //wait for child process 1 to finish
            waitpid(pid2, NULL, 0); //wait for child process 2 to finish
        }
    }
}



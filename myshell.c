/*
    COMP3511 Fall 2022
    PA1: Simplified Linux Shell (MyShell)

    Your name: Aditya Padia
    Your ITSC email:    apadia       @connect.ust.hk

    Declaration:

    I declare that I am not involved in plagiarism
    I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks.

*/

// Note: Necessary header files are included
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> // For constants that are required in open/read/write/close syscalls
#include <sys/wait.h> // For wait() - suppress warning messages
#include <fcntl.h>    // For open/read/write/close syscalls

// Assume that each command line has at most 256 characters (including NULL)
#define MAX_CMDLINE_LEN 256

// Assume that we have at most 8 arguments
#define MAX_ARGUMENTS 8

// Assume that we only need to support 2 types of space characters:
// " " (space) and "\t" (tab)
#define SPACE_CHARS " \t"

// The pipe character
#define PIPE_CHAR "|"

// Assume that we only have at most 8 pipe segements,
// and each segment has at most 256 characters
#define MAX_PIPE_SEGMENTS 8
#define MAX_SEGMENT_LENGTH 256

// Assume that we have at most 8 arguments for each segment
//
// We also need to add an extra NULL item to be used in execvp
//
// Thus: 8 + 1 = 9
//
// Example:
//   echo a1 a2 a3 a4 a5 a6 a7
//
// execvp system call needs to store an extra NULL to represent the end of the parameter list
//
//   char *arguments[MAX_ARGUMENTS_PER_SEGMENT];
//
//   strings stored in the array: echo a1 a2 a3 a4 a5 a6 a7 NULL
//
#define MAX_ARGUMENTS_PER_SEGMENT 9

// Define the  Standard file descriptors here
#define STDIN_FILENO 0  // Standard input
#define STDOUT_FILENO 1 // Standard output

// This function will be invoked by main()
// TODO: Implement the multi-level pipes below
void process_cmd(char *cmdline);

// read_tokens function is given
// This function helps you parse the command line
// Note: Before calling execvp, please remember to add NULL as the last item
void read_tokens(char **argv, char *line, int *numTokens, char *token);

// Here is an example code that illustrates how to use the read_tokens function
// int main() {
//     char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
//     int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
//     char cmdline[MAX_CMDLINE_LEN]; // the input argument of the process_cmd function
//     int i, j;
//     char *arguments[MAX_ARGUMENTS_PER_SEGMENT] = {NULL};
//     int num_arguments;
//     strcpy(cmdline, "ls | sort -r | sort | sort -r | sort | sort -r | sort | sort -r");
//     read_tokens(pipe_segments, cmdline, &num_pipe_segments, PIPE_CHAR);
//     for (i=0; i< num_pipe_segments; i++) {
//         printf("%d : %s\n", i, pipe_segments[i] );
//         read_tokens(arguments, pipe_segments[i], &num_arguments, SPACE_CHARS);
//         for (j=0; j<num_arguments; j++) {
//             printf("\t%d : %s\n", j, arguments[j]);
//         }
//     }
//     return 0;
// }

/* The main function implementation */
int main()
{
    char cmdline[MAX_CMDLINE_LEN];
    fgets(cmdline, MAX_CMDLINE_LEN, stdin);
    process_cmd(cmdline);
    return 0;
}

// TODO: implementation of process_cmd
void process_cmd(char *cmdline)
{
    // You can try to write: printf("%s\n", cmdline); to check the content of cmdline
    if (strcmp(cmdline, "exit\n") == 0)
    {
        printf("The shell program terminates with (pid=%d) terminates \n", getpid());
        exit(1);
    }

    char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
    int num_pipe_segments;                  // an output integer to store the number of pipe segment parsed by this function
    int i, j;
    char *arguments[MAX_ARGUMENTS_PER_SEGMENT] = {NULL};
    int num_arguments;

    int fd;
    char buffer[100];
    char command[MAX_CMDLINE_LEN];

    read_tokens(pipe_segments, cmdline, &num_pipe_segments, PIPE_CHAR);

    // PIPELINE LOOP
    for (int current_pipe = 0; current_pipe < num_pipe_segments; current_pipe++)
    {
        int pdfs[2];
        pid_t pid;
        
        // printf("Current Pipe Segment : %d\n", current_pipe);
        
        if (current_pipe < num_pipe_segments - 1)
        {
            // printf("%s\n", "PIPE CREATED");
            pipe(pdfs);
            // printf("%s\n","FORKED");
            pid = fork();
        }

        if (pid == 0) // CHILD
        {
            // printf("CHILD Pipe Segment :%s  %d\n", pipe_segments[current_pipe], current_pipe);
            if (current_pipe != 0)
            {
                // printf("%s\n", "NOT FIRST PIPE SEGMENT");
                close(STDOUT_FILENO);
                dup2(pdfs[1], STDOUT_FILENO);
                close(pdfs[0]); 
                // dup2(pdfs[0], STDIN_FILENO); 

                // close(STDOUT_FILENO);
                // dup2(pdfs[1], STDOUT_FILENO);
                // close(STDIN_FILENO); 
                // dup2(pdfs[0], STDIN_FILENO); 
                // wait(NULL);
            }
            else {
                // printf("%s\n", "FIRST PIPE SEGMENT");
                close(STDOUT_FILENO);
                dup2(pdfs[1], STDOUT_FILENO);
                close(pdfs[0]);
            }
            // printf("CHILD Pipe Segment :%s\n", pipe_segments[current_pipe]);
            printf("%s\n", "BEFORE READ TOKENS");
            {
            read_tokens(arguments, pipe_segments[current_pipe], &num_arguments, SPACE_CHARS);
            
            // for (j=0; j<num_arguments; j++) {
            //     printf("\t%d : %s\n", j, arguments[j]);
            // }
            strcpy(command, arguments[0]);
            char *temp = arguments[num_arguments - 1];
            if (temp[strlen(temp) - 1] == '\n')
            {
                temp[strlen(temp) - 1] = '\0';
            }
            
            arguments[num_arguments - 1] = temp;

            if (num_arguments == 1)
            {
                command[strlen(command) - 1] = '\0';
                execvp(arguments[0], arguments);
            }
            int redirection_count = 0;

            for (int i = 0; i < num_arguments; i++)
            {
                if (strcmp(arguments[i], "<") == 0 || strcmp(arguments[i], ">") == 0)
                {
                    redirection_count++;
                }
            }

            for (int x = 0; x < redirection_count; x++)
            {
                for (int i = 0; i < num_arguments; i++)
                {
                    if (arguments[i] == NULL)
                    {
                        continue;
                        // break;
                    }
                    if (strcmp(arguments[i], "<") == 0)
                    {
                        fd = open(arguments[i + 1],
                                  O_RDONLY,
                                  S_IRUSR | S_IWUSR);

                        int fd2 = dup2(fd, 0);
                        close(fd);

                        arguments[i] = NULL;
                        arguments[i + 1] = NULL;

                        // for (int k=0; k<MAX_ARGUMENTS_PER_SEGMENT; k++) {
                        //        printf("\t%d : %s\n", k, arguments[k]);
                        // }
                        continue;
                        // execvp(arguments[0], arguments);
                    }

                    if (strcmp(arguments[i], ">") == 0)
                    {
                        fd = open(arguments[i + 1],
                                  O_CREAT | O_WRONLY,
                                  S_IRUSR | S_IWUSR);

                        int fd2 = dup2(fd, 1);
                        close(fd);

                        arguments[i] = NULL;
                        // execvp(arguments[0], arguments);
                        // fflush(stdout);
                    }
                }
            }
            }
            // printf("%s\t%s\n", "ARGUMENT Executed", arguments[0]);
            execvp(arguments[0], arguments);
            break;

        }

        else if (pid > 0) // PARENT
        {
            // printf("PARENT Pipe Segment :%s  %d\n", pipe_segments[current_pipe], current_pipe);
            
            if(current_pipe != num_pipe_segments-1)
            {
                // printf("%s\n", "PARENT NOT LAST PIPE SEGMENT"); 
                close(0);
                dup(pdfs[0]);
                close(pdfs[1]);
                wait(NULL);
                // printf("%s\n", "PARENT FINISHED WAITING FOR CHILD");    
                continue;
            }
            else
            {
                if(current_pipe != 0)
                {
                    // printf("%s\n", "LAST PIPE SEGMENT");
                    close(0);
                    dup2(pdfs[0], 0);
                    close(pdfs[1]);
                    wait(NULL);
                }

            }

            // if (current_pipe == num_pipe_segments - 1)
            // {
                
            // }
            // if (current_pipe != 0)
            // {
            //     close(0);
            //     dup2(pdfs[0], 0);
            //     close(pdfs[1]);

            // }
            {
            read_tokens(arguments, pipe_segments[current_pipe], &num_arguments, SPACE_CHARS);
            // for (j=0; j<num_arguments; j++) {
            //     printf("\t%d : %s\n", j, arguments[j]);
            // }
            strcpy(command, arguments[0]);

            char *temp = arguments[num_arguments - 1];
            temp[strlen(temp) - 1] = '\0';
            arguments[num_arguments - 1] = temp;

            if (num_arguments == 1)
            {
                command[strlen(command) - 1] = '\0';
                execvp(arguments[0], arguments);
            }
            int redirection_count = 0;

            for (int i = 0; i < num_arguments; i++)
            {
                if (strcmp(arguments[i], "<") == 0 || strcmp(arguments[i], ">") == 0)
                {
                    redirection_count++;
                }
            }

            for (int x = 0; x < redirection_count; x++)
            {
                for (int i = 0; i < num_arguments; i++)
                {
                    if (arguments[i] == NULL)
                    {
                        continue;
                        // break;
                    }
                    if (strcmp(arguments[i], "<") == 0)
                    {
                        fd = open(arguments[i + 1],
                                  O_RDONLY,
                                  S_IRUSR | S_IWUSR);

                        int fd2 = dup2(fd, 0);
                        close(fd);

                        arguments[i] = NULL;
                        arguments[i + 1] = NULL;

                        // for (int k=0; k<MAX_ARGUMENTS_PER_SEGMENT; k++) {
                        //        printf("\t%d : %s\n", k, arguments[k]);
                        // }
                        continue;
                        // execvp(arguments[0], arguments);
                    }

                    if (strcmp(arguments[i], ">") == 0)
                    {
                        fd = open(arguments[i + 1],
                                  O_CREAT | O_WRONLY,
                                  S_IRUSR | S_IWUSR);

                        int fd2 = dup2(fd, 1);
                        close(fd);

                        arguments[i] = NULL;
                        // execvp(arguments[0], arguments);
                        // fflush(stdout);
                    }
                }
            }
            }

            // read(pdfs[0], buf, 100);
            // printf("%s\n%s\n", "PARENT OUTPUT", buf);
            // arguments[num_arguments] = buf;
            execvp(arguments[0], arguments);
            fflush(stdout);
            // printf("%s\n", "PARENT Executed");
            break;
            // fflush(stdout);
            // TODO
        }

       
    }
    return;
}

// Implementation of read_tokens function
void read_tokens(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

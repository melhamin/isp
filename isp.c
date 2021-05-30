#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/time.h>

#define ISP_NORMAL_MODE 1
#define ISP_TAPPED_MODE 2
#define BUFF_SIZE 4096
#define DEF_N 256
#define ISP_TOK_DELIM " "
#define ISP_PIPE_C "|"
#define READ 0
#define WRITE 1
#define FAIL_PIPE "Failed to create pipe!"
#define FAIL_FORK "Failed to fork!"
#define KBLU "\x1B[34m"
#define KNRM "\x1B[0m"
#define KRED "\x1b[31m"
#define KGRN "\x1b[32m"

// isp funcs
void execute_command(char *command);
void isp_simple(char *command);
void isp_compound_normal(char *command);
void isp_print_stats(size_t char_count, size_t read_count, size_t write_count);
void isp_compound_tapped(char *command, int n);
void isp_exec_command(char *c, int mode, int n);
char *isp_get_input();

// Util funcs
void parse_args(int argc, char **argv, int *mode, int *n);
char **split_str(char *str, const char *delim);
char *trim(char *str);

int main(int argc, char **argv)
{
    int mode;
    int n;
    parse_args(argc, argv, &mode, &n);

    char *command;
    do
    {
        command = isp_get_input();
        isp_exec_command(command, mode, n);
        free(command);

    } while (1);

    return 0;
}

char *isp_get_input()
{
    char *input = malloc(sizeof(char *) * BUFF_SIZE);

    printf(KBLU "isp$ " KNRM);
    scanf(" %[^\n]", input);

    if (strcmp(trim(input), "exit") == 0)
        exit(0);
    return input;
}

// Parses the command line arguments
void parse_args(int argc, char **argv, int *mode, int *n)
{
    if (argc > 2)
    {
        *n = atoi(argv[1]);
        *mode = atoi(argv[2]);
        char *M = *mode == ISP_NORMAL_MODE ? "Normal" : "Tapped";

        if (*n > 4096)
        {
            printf(KRED "");
            printf("[-] N must be <= 4096. Using default value...\n");
            *n = DEF_N;
            printf(KNRM "");
        }
        printf(KGRN "");
        printf("[*] N: %d, MODE: %s\n", *n, M);
        printf(KNRM "");
    }
    else
    {
        *mode = 1;
        *n = DEF_N;
        char *M = *mode == 1 ? "Normal" : "Tapped";
        printf(KRED "");
        printf("[*] Invalid or no args passed! Using defaults:\n[*] N: %d, MODE: %s\n", *n, M);
        printf(KNRM "");
    }
}

// Kills the process in case of error and 
// writes the given error message to the 
// standard output
void isp_exit(char *error_msg)
{
    fprintf(stderr, "%s", error_msg);
    exit(EXIT_FAILURE);
}

// Executes the given command using exec*
void execute_command(char *command)
{
    char **args = split_str(command, ISP_TOK_DELIM);
    execvp(args[0], args);
    printf("[-] Commmand '%s' not found.\n", args[0]);
    fflush(stdout);
    exit(EXIT_FAILURE);
}

// Executes simple commands (no pipes)
void isp_simple(char *command)
{
    size_t id = fork();

    if (id < 0)
        isp_exit(FAIL_FORK);

    if (id == 0)
        execute_command(command);
    else
        wait(NULL);
}

// Measures the time elapsed to perform
// the command and gives some statistics.
void calc_elapsed_time(struct timeval start, struct timeval end)
{
    double elapsed_time;
    elapsed_time = (end.tv_sec - start.tv_sec) * 1000.0;    // sec to ms
    elapsed_time += (end.tv_usec - start.tv_usec) / 1000.0; // us to ms
    printf("\n------------------------------\n");
    printf("Elapsed time: %f ms.\n", elapsed_time);
    printf("------------------------------\n");
}

// Executes the commands that have | in them by creating two
// processes and redirecting output of one to the input of other
void isp_compound_normal(char *command)
{
    // struct timeval start_time, end_time;
    int fds[2];
    size_t id1, id2;

    // Split command into two (before and after '|')
    char **com = split_str(command, ISP_PIPE_C);

    char *first_command = trim(com[0]);
    char *second_command = trim(com[1]);

    // Start the timer
    // gettimeofday(&start_time, NULL);

    if (pipe(fds) < 0)
        isp_exit(FAIL_PIPE);

    id1 = fork();
    if (id1 < 0)
        isp_exit(FAIL_FORK);

    else if (id1 == 0)
    {
        close(fds[READ]);
        dup2(fds[WRITE], STDOUT_FILENO);
        close(fds[WRITE]);
        execute_command(first_command);
    }

    id2 = fork();
    if (id2 < 0)
        isp_exit(FAIL_FORK);

    else if (id2 == 0)
    {
        close(fds[WRITE]);
        dup2(fds[READ], STDIN_FILENO);
        close(fds[READ]);
        execute_command(second_command);
    }

    close(fds[READ]);
    close(fds[WRITE]);
    for (int i = 0; i < 2; i++)
        wait(NULL);
    // gettimeofday(&end_time, NULL);
    // calc_elapsed_time(start_time, end_time);
    free(com);
}

void isp_print_stats(size_t char_count, size_t read_count, size_t write_count)
{
    printf("\n------------------------\n");
    printf("Character Count: %zu\n", char_count);
    printf("Read-Call Count: %zu\n", read_count);
    printf("Write-Call Count: %zu\n", write_count);
    printf("------------------------\n");
    fflush(stdout);
}

// Executes the commands that are in tapped mode
// and are compund commands (pipes)
void isp_compound_tapped(char *command, int n)
{
    // struct timeval start_time, end_time;
    int pipe1[2], pipe2[2];
    pid_t pid1, pid2;
    size_t char_count = 0, read_count = 0, write_count = 0;

    // Split command into two (before and after '|')
    char **com = split_str(command, ISP_PIPE_C);

    char *first_command = trim(com[0]);
    char *second_command = trim(com[1]);

    int p1 = pipe(pipe1);
    int p2 = pipe(pipe2);

    if (p1 < 0 || p2 < 0)
        isp_exit(FAIL_PIPE);

    // Start the timer
    // gettimeofday(&start_time, NULL);

    // Create first child
    pid1 = fork();
    if (pid1 < 0)
        isp_exit(FAIL_FORK);

    else if (pid1 == 0)
    {
        close(pipe2[READ]);
        close(pipe2[WRITE]);
        dup2(pipe1[WRITE], STDOUT_FILENO);
        close(pipe1[READ]);
        close(pipe1[WRITE]);
        execute_command(first_command);
    }

    // create second child
    pid2 = fork();
    if (pid2 < 0)
        isp_exit(FAIL_FORK);

    else if (pid2 == 0)
    {
        close(pipe1[READ]);
        close(pipe1[WRITE]);
        close(pipe2[WRITE]);
        dup2(pipe2[READ], STDIN_FILENO);
        close(pipe2[READ]);
        execute_command(second_command);
    }

    // Read from pipe1 and write to pipe 2
    char buff[n];
    close(pipe1[WRITE]);
    close(pipe2[READ]);

    size_t bytes_read;

    while ((bytes_read = read(pipe1[READ], &buff, n)) > 0)
    {
        read_count++;
        write(pipe2[WRITE], &buff, bytes_read);
        write_count++;
        char_count += bytes_read;
    }

    // Close pipes
    close(pipe1[READ]);
    close(pipe1[WRITE]);
    close(pipe2[READ]);
    close(pipe2[WRITE]);
    for (int i = 0; i < 2; i++)
        wait(NULL);

    // End the timer
    // gettimeofday(&end_time, NULL);
    // Show statistics
    isp_print_stats(char_count, read_count, write_count);
    // calc_elapsed_time(start_time, end_time);

    // Free up the memory allocated for the command in split_str function
    free(com);
}

// Executes the given command by calling
// their corresponding functions with respect
// to their mode.
void isp_exec_command(char *c, int mode, int n)
{
    int hasPipe = 0;
    char *command = trim(c);
    char *tmp = command;

    while (*tmp)
    {
        if (*tmp++ == '|')
            hasPipe = 1;
    }

    if (!hasPipe)
        isp_simple(command);
    else
    {
        if (mode == 2)
            isp_compound_tapped(command, n);
        else
            isp_compound_normal(command);
    }
}

// Splits the given string wrt the given delimiter.
char **split_str(char *str, const char *delim)
{
    size_t count = 0;
    char *tmp = str;
    char **result;

    // Count the number of delimiters
    while (*tmp)
    {
        if (*tmp == *delim)
        {
            count++;
        }
        tmp++;
    }

    count += 2;

    result = malloc(sizeof(char *) * (count));

    // Get first token
    char *token = strtok(str, delim);
    size_t i = 0;
    // add tokens to the array
    while (token != NULL)
    {
        result[i++] = strdup(token);
        token = strtok(NULL, delim);
    }

    // Terminate the array with NULL for execvp call
    result[count - 1] = '\0';

    return result;
}

// Removes the wihtespace from the given string.
char *trim(char *str)
{
    char *end;
    while (isspace(*str))
        str++;

    end = str + strlen(str) - 1;
    while (end > str && isspace(*end))
        end--;

    *(end + 1) = '\0';
    return str;
}
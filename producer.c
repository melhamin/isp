#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int rand_num(int lower, int upper)
{
    int res = rand() % (upper - lower + 1) + lower;
    return res;
}

void parse_args(int argc, char **argv, int *num_of_chars)
{
    if (argc < 2)
        *num_of_chars = 10;
    else
        *num_of_chars = atoi(argv[1]);
}

int main(int argc, char **argv)
{
    char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int num_of_chars;
    int count = 0;

    // printf("%d --- %s\n", argc, argv[1]);
    // return 0;

    srand(time(0));

    parse_args(argc, argv, &num_of_chars);

    while (count < num_of_chars)
    {
        int ind = rand_num(1, sizeof(charset) - 1);
        printf("%c", charset[ind]);
        count++;
    }
}
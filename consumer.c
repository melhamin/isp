#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void parse_args(int argc, char **argv, int *num_of_chars)
{
    if (argc < 2)
        *num_of_chars = 10;
    else
        *num_of_chars = atoi(argv[1]);
}

int main(int argc, char **argv)
{
    char ch;
    int num_of_chars;
    int count = 0;

    parse_args(argc, argv, &num_of_chars);

    while (count < num_of_chars)
    {
        scanf("%c", &ch);
        printf("%c", ch);
        count++;
    }
}
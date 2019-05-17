#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    pid_t parentPid = getpid();

    pid_t childPid;

    switch (fork())
    {
        case -1:
            exit(EXIT_FAILURE);
        case 0: // Child
            break;
        default: // Parent
            break;
    }

    return EXIT_SUCCESS;
}
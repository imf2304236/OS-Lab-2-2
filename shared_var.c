#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

volatile pid_t parentPid, childPid;

int main()
{
    int forkResult;
    parentPid = getpid();

    forkResult = fork();
    switch (forkResult)
    {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
        case 0:
            sleep(1);
            printf("[PID %ld] Parent PID = %ld\n", (long) getpid(), (long) parentPid);
            printf("[PID %ld] Child PID = %ld\n", (long) getpid(), (long) childPid);
            break;
        default:
            childPid = forkResult;
            printf("[PID %ld] Parent PID = %ld\n", (long) getpid(), (long) parentPid);
            printf("[PID %ld] Child PID = %ld\n", (long) getpid(), (long) childPid);
            wait(&childPid);
    }

    exit(EXIT_SUCCESS);
}
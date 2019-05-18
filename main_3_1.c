#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#define MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#define TEXT "THIS IS NEW TEXT!\0"
#define FILE "testfile.txt"

static void sigh(int);

volatile bool running = false;

int main(void)
{
    int fd, flags;
    bool running = true;
    struct sigaction sa;

    // Configure signal handler
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = sigh;
    if (sigaction(SIGIO, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (fd = open(FILE, O_CREAT | O_WRONLY, MODE) == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (write(fd, TEXT, strlen(TEXT)) <= 0)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }

    if (close(fd) == -1)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }

    if (fd = open(FILE, O_RDONLY | O_SYNC | O_RSYNC) == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Set process as signal receiver
    if (fcntl(fd, F_SETOWN, getpid()) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }

    // Enable signal
    flags = fcntl(fd, F_GETFL);
    if (fcntl(fd, F_SETFL, flags | O_ASYNC | O_NONBLOCK) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }

    while(running) {
        // pause();

        // fd = open(FILE, O_RDONLY);
        char buffer[BUFSIZ] = "\0";
        // read(fd, buffer, BUFSIZ);
        printf("[PID %ld] Read \"%s\" from file.\n", (long) getpid(), buffer);

        sleep(1);

        // closing and removing datafile
        close(fd);
        fprintf(stdout, "PARENT: file closed\n");
        remove("testfile.txt");
        return EXIT_SUCCESS;
    }
}

void sigh(int signo) {
    // if (signo == SIGINT)
        // running = false;
}
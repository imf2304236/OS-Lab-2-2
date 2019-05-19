#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#define MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define TEXT "THIS IS NEW TEXT!\0"

void sigh(int);

volatile bool running = false;

int main(void)
{
    int fd;
    bool running = true;
    bool done = false;
    char ch;
    int cnt = 0;

    signal(SIGINT, sigh);

    // creating datafile
    fd = open("testfile.txt", O_CREAT | O_RDWR | O_TRUNC | O_SYNC, MODE);
    if (fd == -1) {
        fprintf (stderr, "cannot create file testfile.txt - %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    fprintf (stdout, "datafile \"testfile.txt\" created successfully\n");

    ssize_t bytesWritten = write(fd, TEXT, strlen(TEXT));
    if (bytesWritten == 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    printf("[PID %ld] Text written to file.\n", (long) getpid());

    while(!done)
    {
        while (read(fd, &ch, 1) > 0 && !done) {
            printf("cnt=%d; read %c\n", cnt, ch);
            done = ch == '#';
        }
    }

    // closing and removing datafile
    close(fd);
    fprintf (stdout, "PARENT: file closed\n");
    remove("testfile.txt");

    return EXIT_SUCCESS;
}

void sigh(int signo)
{
    running = false;
}
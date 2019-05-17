#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <stdbool.h>

#define FILE "/Users/fennie/Library/Mobile Documents/com~apple~CloudDocs/Documents/School/HAW/19SS/OS/Lab/Lab 2/Lab_2_2_0/file2.txt"
#define TEXT "THIS IS NEW TEXT!\0"

void sigh(int sig);
char *mapAnonSharedVariable(void);
int openFile(int flags);
void lockFile(int fd);
void unlockFile(int fd);
void writeToFile(int fd);
ssize_t readFromFile(int fd);
void eraseFile(int fd);
void closeFile(int fd);
void readProcess(int fd, const char *fileWritten, char *fileRead);
void writeEraseProcess(int fd, char *fileWritten, char *fileRead, char* togglePtr);

volatile bool running = true;


int main(int argc, char *argv[])
{
    int fd;

    signal(SIGINT, sigh);

    setbuf(stdout, NULL); /* Disable buffering of stdout */

    char *togglePtr = mapAnonSharedVariable();
    *togglePtr = 1;

    char *fileRead = mapAnonSharedVariable();
    *fileRead = 0;

    char *fileWritten = mapAnonSharedVariable();
    *fileWritten = 0;

    switch (fork())
    {
        case -1:
            exit(EXIT_FAILURE);
        case 0: // Child
            fd = openFile(O_RDWR);

            while(running)
            {
                if (*togglePtr && !*fileRead)
                    readProcess(fd, fileWritten, fileRead);
                else if (!*togglePtr)
                    writeEraseProcess(fd, fileWritten, fileRead, togglePtr);
            }
            close(fd);

            break;
        default: // Parent
            fd = openFile(O_RDWR);

            while(running)
            {
                if (*togglePtr)
                    writeEraseProcess(fd, fileWritten, fileRead, togglePtr);
                else if (!*togglePtr && !*fileRead)
                    readProcess(fd, fileWritten, fileRead);
            }
            close(fd);
    }

    return EXIT_SUCCESS;
}


void readProcess(int fd, const char *fileWritten, char *fileRead)
{
    if (*fileWritten)
    {
        lockFile(fd);
        if (readFromFile(fd))
            *fileRead = 1;
        unlockFile(fd);
    }
}


void writeEraseProcess(int fd, char *fileWritten, char *fileRead, char* togglePtr)
{
    if (!*fileWritten && !*fileRead)
    {
        lockFile(fd);
        writeToFile(fd);
        *fileWritten = 1;
        unlockFile(fd);
    }

    if (*fileWritten && *fileRead)
    {
        lockFile(fd);
        eraseFile(fd);

        // toggle
        *fileWritten = 0;
        *fileRead = 0;
        *togglePtr = (*togglePtr) ? (char) 0 : (char) 1;

        unlockFile(fd);
    }
}


void sigh(int sig)
{
    if (sig == SIGINT)
    {
        running = false;
    }
}


char *mapAnonSharedVariable(void)
{
    char *togglePtr = mmap(NULL, sizeof(volatile bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (togglePtr == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return togglePtr;
}


int openFile(int flags)
{
    int fd = open(FILE, flags);
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    return fd;
}


void lockFile(int fd)
{
    if (flock(fd, LOCK_EX) == -1)
    {
        perror("flock");
        exit(EXIT_FAILURE);
    }
    printf("[PID %ld] File locked.\n", (long) getpid());
}


void unlockFile(int fd)
{
    if (flock(fd, LOCK_UN) == -1)
    {
        perror("flock");
        exit(EXIT_FAILURE);
    }
    printf("[PID %ld] File unlocked.\n", (long) getpid());
}


void writeToFile(int fd)
{
    ssize_t bytesWritten = write(fd, TEXT, strlen(TEXT));
    if (bytesWritten == 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    printf("[PID %ld] Text written to file.\n", (long) getpid());
}


ssize_t readFromFile(int fd)
{
    char buffer[BUFSIZ] = "\0";
    ssize_t bytesRead = read(fd, buffer, BUFSIZ);

    if (bytesRead == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    else if (bytesRead > 0)
    {
        printf("[PID %ld] Read \"%s\" from file.\n", (long) getpid(), buffer);
    }
    else if (!bytesRead)
    {
        printf("[PID %ld] EMPTY READ.\n", (long) getpid());
    }

    return bytesRead;
}

void eraseFile(int fd)
{
    if (ftruncate(fd, 0) == -1)
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    printf("[PID %ld] Truncated file.\n", (long) getpid());
}

void closeFile(int fd)
{
    if (close(fd) == -1)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
}

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define BUF_SIZE 1000
#define FILE "/Users/fennie/Library/Mobile Documents/com~apple~CloudDocs/Documents/School/HAW/19SS/OS/Lab/Lab 2/Lab_2_2_0/file2.txt"
#define N_CYCLES 4
#define SYNC_SIG SIGUSR1 /* Synchronization signal */
#define TEXT "THIS IS NEW TEXT!\0"

volatile bool running = false;

int openFile(int flags);
void writeToFile(int fd);
void readFromFile(int fd);
void eraseFile(const char*);
void closeFile(int fd);
void writeEraseProcess(pid_t, char *);
void readProcess(pid_t);

void sigh(int sig)
{
    if (sig == SIGINT)
    {
        running = false;
    }
}

void errExit(const char* arg)
{
    perror(arg);
    _exit(EXIT_FAILURE);
}


int main(int argc, char *argv[]) {
    pid_t childPid;
    running = true;

    signal(SIGINT, sigh);

    setbuf(stdout, NULL); /* Disable buffering of stdout */

    char *togglePtr = mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (togglePtr == MAP_FAILED)
        errExit("mmap");

    *togglePtr = 1;

    signal(SYNC_SIG, sigh);

    switch (childPid = fork()) {
        case -1:
            errExit("fork");
        case 0: /* Child */
            for (int i = 0; i!=N_CYCLES && running; ++i)
            {
                if (*togglePtr)
                    writeEraseProcess(getppid(), togglePtr);
                else
                    readProcess(getppid());
            }

            if (munmap(togglePtr, sizeof(int)) == -1)
                errExit("munmap");
            exit(EXIT_SUCCESS);

        default: /* Parent */
            for (int i = 0; i!=N_CYCLES && running; ++i)
                {
                    if (*togglePtr)
                        readProcess(childPid);
                    else
                        writeEraseProcess(childPid, togglePtr);
                }

            if (munmap(togglePtr, sizeof(int)) == -1)
                errExit("munmap");
            exit(EXIT_SUCCESS);
    }
}


void writeEraseProcess(pid_t otherPid, char *toggleAddr)
{
    // WRITE FILE
    int fd = openFile(O_WRONLY);
    writeToFile(fd);
    closeFile(fd);

    // SEND SIGNAL
    if (kill(otherPid, SYNC_SIG) == -1)
        errExit("kill");

    // WAIT FOR SIGNAL
    pause();

    // ERASE FILE
    eraseFile(FILE);

    // TOGGLE
    *toggleAddr = (*toggleAddr) ? (char) 0 : (char) 1;

    // SEND SIGNAL
    if (kill(otherPid, SYNC_SIG) == -1)
        errExit("kill");
}


void readProcess(pid_t otherPid)
{
    int fd;

    // WAIT FOR SIGNAL
    pause();

    // READ FILE
    fd = openFile(O_RDONLY);
    readFromFile(fd);
    closeFile(fd);

    // SEND SIGNAL
    if (kill(otherPid, SYNC_SIG) == -1)
        errExit("kill");

    // WAIT FOR SIGNAL
    pause();
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


void writeToFile(int fd)
{
    ssize_t bytesWritten = write(fd, TEXT, strlen(TEXT));
    if (bytesWritten == 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    printf("[PID %ld] Text written to file.\n", (long) getpid());
}


void readFromFile(int fd)
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
}


void eraseFile(const char *file)
{
    int fd = open(file, O_RDWR | O_TRUNC);
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    printf("[PID %ld] File truncated.\n", (long) getpid());
    closeFile(fd);
}


void closeFile(int fd)
{
    if (close(fd) == -1)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
}
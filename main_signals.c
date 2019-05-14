#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define BUF_SIZE 1000
#define FILE "/Users/fennie/Library/Mobile Documents/com~apple~CloudDocs/Documents/School/HAW/19SS/OS/Lab/Lab 2/Lab_2_2_0/file2.txt"
#define N_CYCLES 2
#define SYNC_SIG SIGUSR1 /* Synchronization signal */
#define TEXT "THIS IS NEW TEXT!\0"

bool running = false;

int openFile(int flags);
int writeToFile(int fd);
int readFromFile(int fd);
void eraseFile(const char*);
void closeFile(int fd);

static void handler(int sig)
/* Signal handler - does nothing but return */
{
}

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


char * currTime(const char *format)
/* Return a string containing the current time formatted according to
the specification in 'format' (see strftime(3) for specifiers).
If 'format' is NULL, we use "%c" as a specifier (which gives the
date and time as for ctime(3), but without the trailing newline).
Returns NULL on error. */
{
    static char buf[BUF_SIZE]; /* Nonreentrant */
    time_t t;
    size_t s;
    struct tm *tm;
    t = time(NULL);
    tm = localtime(&t);
    if (tm == NULL)
        return NULL;
    s = strftime(buf, BUF_SIZE, (format != NULL) ? format : "%c", tm);
    return (s == 0) ? NULL : buf;
}


int main(int argc, char *argv[]) {
    int fd;
    pid_t childPid;
    sigset_t blockMask, origMask, emptyMask;
    struct sigaction sa;
    volatile bool running = true;

    signal(SIGINT, sigh);

    setbuf(stdout, NULL); /* Disable buffering of stdout */

    char *toggleAddr = mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (toggleAddr == MAP_FAILED)
        errExit("mmap");

    *toggleAddr = 1;

    sigemptyset(&blockMask);
    sigaddset(&blockMask, SYNC_SIG); /* Block signal */
    if (sigprocmask(SIG_BLOCK, &blockMask, &origMask) == -1)
        errExit("sigprocmask");

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handler;
    if (sigaction(SYNC_SIG, &sa, NULL) == -1)
        errExit("sigaction");

    switch (childPid = fork()) {
        case -1:
            errExit("fork");
        case 0: /* Child */
//            while(running)
            for (int i = 0; i!=N_CYCLES && running; ++i)
            {
                if (*toggleAddr)
                {
                    printf("[%s %ld] Child started.\n", currTime("%T"), (long) getpid());

                    // WRITE FILE
                    fd = openFile(O_WRONLY);
                    writeToFile(fd);
                    closeFile(fd);


                    // SEND SIGNAL
                    /* And then signals parent that it's done */
                    printf("[%s %ld] Signaling parent...\n", currTime("%T"), (long) getpid());
                    if (kill(getppid(), SYNC_SIG) == -1)
                        errExit("kill");
                    printf("[%s %ld] Signaled parent.\n", currTime("%T"), (long) getpid());

                    // WAIT FOR SIGNAL
                    /* Now child can do other things... */
                    printf("[%s %ld] Child waiting for signal...\n",
                           currTime("%T"), (long) getpid());
                    sigemptyset(&emptyMask);
                    if (sigsuspend(&emptyMask) == -1 && errno != EINTR)
                        errExit("sigsuspend");
                    printf("[%s %ld] Child got signal.\n", currTime("%T"), (long) getpid());

                    /* If required, return signal mask to its original state */
                    if (sigprocmask(SIG_SETMASK, &origMask, NULL) == -1)
                        errExit("sigprocmask");

                    // ERASE FILE
                    eraseFile(FILE);
                    *toggleAddr = 0;

                    // SEND SIGNAL
                    printf("[%s %ld] Signaling parent...\n", currTime("%T"), (long) getpid());
                    if (kill(getppid(), SYNC_SIG) == -1)
                        errExit("kill");
                    printf("[%s %ld] Signaled parent.\n", currTime("%T"), (long) getpid());
                }
                else
                {
                    printf("[%s %ld] Child started.\n", currTime("%T"), (long) getpid());
                    /* Waits for parent to write to file*/

                    // WAIT FOR SIGNAL
                    printf("[%s %ld] Child waiting for signal...\n",
                           currTime("%T"), (long) getpid());
                    sigemptyset(&emptyMask);
                    if (sigsuspend(&emptyMask) == -1 && errno != EINTR)
                        errExit("sigsuspend");
                    printf("[%s %ld] Child got signal.\n", currTime("%T"), (long) getpid());

                    /* If required, return signal mask to its original state */
                    if (sigprocmask(SIG_SETMASK, &origMask, NULL) == -1)
                        errExit("sigprocmask");

                    // READ FILE
                    fd = openFile(O_RDONLY);
                    readFromFile(fd);
                    closeFile(fd);

                    // SEND SIGNAL
                    /* And then signals parent that it's done */
                    printf("[%s %ld] Signaling parent...\n",
                           currTime("%T"), (long) getpid());
                    if (kill(getppid(), SYNC_SIG) == -1)
                        errExit("kill");
                    printf("[%s %ld] Signaled parent.\n", currTime("%T"), (long) getpid());

                    // WAIT FOR SIGNAL
                    printf("[%s %ld] Child waiting for signal...\n",
                           currTime("%T"), (long) getpid());
                    sigemptyset(&emptyMask);
                    if (sigsuspend(&emptyMask) == -1 && errno != EINTR)
                        errExit("sigsuspend");
                    printf("[%s %ld] Child got signal.\n", currTime("%T"), (long) getpid());

                    /* If required, return signal mask to its original state */
                    if (sigprocmask(SIG_SETMASK, &origMask, NULL) == -1)
                        errExit("sigprocmask");
                }
            }

            if (munmap(toggleAddr, sizeof(int)) == -1)
                errExit("munmap");
            exit(EXIT_SUCCESS);


        default: /* Parent */
//            while(running)
            for (int i = 0; i!=N_CYCLES && running; ++i)
            {
                if (*toggleAddr) {

                    printf("[%s %ld] Parent started.\n", currTime("%T"), (long) getpid());
                    /* Waits for child to write to file*/

                    // WAIT FOR SIGNAL
                    printf("[%s %ld] Parent waiting for signal...\n",
                           currTime("%T"), (long) getpid());
                    sigemptyset(&emptyMask);
                    if (sigsuspend(&emptyMask) == -1 && errno != EINTR)
                        errExit("sigsuspend");
                    printf("[%s %ld] Parent got signal.\n", currTime("%T"), (long) getpid());

                    /* If required, return signal mask to its original state */
                    if (sigprocmask(SIG_SETMASK, &origMask, NULL) == -1)
                        errExit("sigprocmask");

                    // READ FILE
                    fd = openFile(O_RDONLY);
                    readFromFile(fd);
                    closeFile(fd);

                    // SEND SIGNAL
                    /* And then signals parent that it's done */
                    printf("[%s %ld] Signaling child...\n",
                           currTime("%T"), (long) getpid());
                    if (kill(childPid, SYNC_SIG) == -1)
                        errExit("kill");
                    printf("[%s %ld] Signaled child.\n", currTime("%T"), (long) getpid());

                    // WAIT FOR SIGNAL
                    printf("[%s %ld] Parent waiting for signal...\n",
                           currTime("%T"), (long) getpid());
                    sigemptyset(&emptyMask);
                    if (sigsuspend(&emptyMask) == -1 && errno != EINTR)
                        errExit("sigsuspend");
                    printf("[%s %ld] Parent got signal.\n", currTime("%T"), (long) getpid());
                }
                else
                {
                    printf("[%s %ld] Parent started.\n", currTime("%T"), (long) getpid());

                    // WRITE FILE
                    fd = openFile(O_WRONLY);
                    writeToFile(fd);
                    closeFile(fd);


                    // SEND SIGNAL
                    /* And then signals parent that it's done */
                    printf("[%s %ld] Signaling child...\n", currTime("%T"), (long) getpid());
                    if (kill(childPid, SYNC_SIG) == -1)
                        errExit("kill");
                    printf("[%s %ld] Signaled child.\n", currTime("%T"), (long) getpid());

                    // WAIT FOR SIGNAL
                    /* Now child can do other things... */
                    printf("[%s %ld] Parent waiting for signal...\n",
                           currTime("%T"), (long) getpid());
                    sigemptyset(&emptyMask);
                    if (sigsuspend(&emptyMask) == -1 && errno != EINTR)
                        errExit("sigsuspend");
                    printf("[%s %ld] Parent got signal.\n", currTime("%T"), (long) getpid());

                    // ERASE FILE
                    eraseFile(FILE);
                    *toggleAddr = 1;

                    // SEND SIGNAL

                    /* And then signals child that it's done */
                    printf("[%s %ld] Signaling child...\n", currTime("%T"), (long) getpid());
                    if (kill(childPid, SYNC_SIG) == -1)
                        errExit("kill");
                    printf("[%s %ld] Signaled child.\n", currTime("%T"), (long) getpid());
                }
            }

            if (munmap(toggleAddr, sizeof(int)) == -1)
                errExit("munmap");
            exit(EXIT_SUCCESS);
    }
}


int openFile(int flags)
{
    int fd;
    printf("[%s %ld] Opening file...\n", currTime("%T"), (long) getpid());
    fd = open(FILE, flags);
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    printf("[%s %ld] File opened.\n", currTime("%T"), (long) getpid());
    return fd;
}


int writeToFile(int fd)
{
    int bytesWritten;

    printf("[%s %ld] Writing text to file...\n", currTime("%T"), (long) getpid());
    bytesWritten = write(fd, TEXT, strlen(TEXT));

    if (bytesWritten == 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    printf("[%s %ld] Text written to file.\n", currTime("%T"), (long) getpid());

    return bytesWritten;
}


int readFromFile(int fd)
{
    int bytesRead;
    int result;
    char buffer[BUFSIZ] = "\0";

    printf("[%s %ld] Reading text from file...\n", currTime("%T"), (long) getpid());
    bytesRead = read(fd, buffer, BUFSIZ);
    printf("[%s %ld] %d bytes read.\n", currTime("%T"), (long) getpid(), bytesRead);

    if (bytesRead == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    else if (bytesRead > 0)
    {
        printf("[%s %ld] Buffer = %s\n", currTime("%T"), (long) getpid(), buffer);
        return bytesRead;
    }

    return bytesRead;
}


void eraseFile(const char *file)
{
    printf("[%s %ld] Truncating file...\n", currTime("%T"), (long) getpid());
    int fd = open(file, O_RDWR | O_TRUNC);
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    printf("[%s %ld] File truncated.\n", currTime("%T"), (long) getpid());
    closeFile(fd);
}


void closeFile(int fd)
{
    int result;
    printf("[%s %ld] Closing file...\n", currTime("%T"), (long) getpid());
    result = close(fd);
    if (result == -1)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
    printf("[%s %ld] File closed.\n", currTime("%T"), (long) getpid());
}
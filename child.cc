#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>

using namespace std;

#define READ 0
#define WRITE 1

#define TO_KERNEL 3
#define FROM_KERNEL 4

#define BUFFER_SIZE 1024

int lock = 1;

void unlock(int signum)
{
    cout << "Unlocked from child (" << getpid() << ")." << endl;
    lock = 0;
}

int main (int argc, char** argv)
{
    signal(SIGUSR1, unlock);
	int fileDes = atoi(argv[0]);
	char buffer[BUFFER_SIZE];

	char* message = "p";
	if(write((fileDes + TO_KERNEL), message, strlen(message)) == -1)
    {
        perror("write");
    }

	if(kill(getppid(), SIGTRAP) < 0)
    {
        perror("kill");
    }

	while(lock){
        sleep(1);
    }

	if(read(fileDes, buffer, BUFFER_SIZE) == -1)
    {
        perror("read");
    }

	if(write(1, buffer, strlen(buffer)) == -1)
    {
        perror("write");
    }
}

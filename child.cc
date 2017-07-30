#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>

bool lock = true;

using namespace std;

#define TO_KERNEL 3
#define FROM_KERNEL 4

#define BUFFER_SIZE 1024

void unlock(int signum)
{
    cout << "Unlocked from child (" << getpid() << ")." << endl;
    lock = false;
}

int main (int argc, char** argv)
{
    cout << "Entered child (" << getpid() << ")." << endl
         << "Argv: " << argv[0] << endl;
    signal(SIGUSR1, unlock);

	int comm_addr = atoi(argv[0]);
	char buffer[BUFFER_SIZE];
	char* message = "p";

	if(write((comm_addr + TO_KERNEL), message, strlen(message)) == -1)
    {
        perror("write");
    }
    else
    {
        cout << "Child: Writing (" << message << ") successful." << endl;
    }

	if(kill(getppid(), SIGTRAP) < 0)
    {
        perror("kill");
    }
    else
    {
        cout << "Child: Killing to (" << getppid() << ") successful." << endl;
    }

    cout << "Child sleeping..." << endl;
	do {
        sleep(1);
    } while(lock);
    cout << "Child waking up." << endl;

	if(read(comm_addr, buffer, BUFFER_SIZE) == -1)
    {
        perror("read");
        cout << "Read error in file: " << __FILE__ << endl;
    }

	if(write(1, buffer, strlen(buffer)) == -1)
    {
        perror("write");
    }
}

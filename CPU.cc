/*
 *  Programmer:         Zachary Champion
 *  Project:            CS 3600 Homework 3
 *  Description:        Virtual CPU
 *  Date Last Updated:  27 July 2017
 *  Sources & help:     Collaborated with Joey in the CS room.
*/

#include <iostream>
#include <list>
#include <iterator>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>


#define NUM_SECONDS 20

// make sure the asserts work
#undef NDEBUG
#include <cassert>

#define EBUG
#ifdef EBUG
#   define dmess(a) cout << "in " << __FILE__ << \
    " at " << __LINE__ << " " << (a) << endl;

#   define dprint(a) cout << "in " << __FILE__ << \
    " at " << __LINE__ << " " << (#a) << " = " << (a) << endl;

#   define dprintt(a,b) cout << "in " << __FILE__ << \
    " at " << __LINE__ << " " << (a) << " " << (#b) << " = " \
    << (b) << endl
#else
#   define dprint(a)
#endif /* EBUG */

using namespace std;

enum STATE { NEW, RUNNING, WAITING, READY, TERMINATED };

/*
** a signal handler for those signals delivered to this process, but
** not already handled.
*/
void grab (int signum) { dprint (signum); }

// c++decl> declare ISV as array 32 of pointer to function (int) returning
// void
void (*ISV[32])(int) = {
/*        00    01    02    03    04    05    06    07    08    09 */
/*  0 */ grab, grab, grab, grab, grab, grab, grab, grab, grab, grab,
/* 10 */ grab, grab, grab, grab, grab, grab, grab, grab, grab, grab,
/* 20 */ grab, grab, grab, grab, grab, grab, grab, grab, grab, grab,
/* 30 */ grab, grab
};

struct PCB
{
    STATE state;
    const char *name;   // name of the executable
    int pid;            // process id from fork();
    int ppid;           // parent process id
    int interrupts;     // number of times interrupted
    int switches;       // may be < interrupts
    int started;        // the time this process started
};

/*
** an overloaded output operator that prints a PCB
*/
ostream& operator << (ostream &os, struct PCB *pcb)
{
    os << "state:        " << pcb->state << endl;
    os << "name:         " << pcb->name << endl;
    os << "pid:          " << pcb->pid << endl;
    os << "ppid:         " << pcb->ppid << endl;
    os << "interrupts:   " << pcb->interrupts << endl;
    os << "switches:     " << pcb->switches << endl;
    os << "started:      " << pcb->started << endl;
    return (os);
}

/*
** an overloaded output operator that prints a list of PCBs
*/
ostream& operator << (ostream &os, list<PCB *> which)
{
    list<PCB *>::iterator PCB_iter;
    for (PCB_iter = which.begin(); PCB_iter != which.end(); PCB_iter++)
    {
        os << (*PCB_iter);
    }
    return (os);
}

PCB *running;
PCB *idle;

// http://www.cplusplus.com/reference/list/list/
list<PCB *> new_list;
list<PCB *> processes;

int sys_time;

/*
**  send signal to process pid every interval for number of times.
*/
void send_signals (int signal, int pid, int interval, int number)
{
    dprintt ("at beginning of send_signals", getpid ());

    for (int i = 1; i <= number; i++)
    {
        sleep (interval);

        dprintt ("sending", signal);
        dprintt ("to", pid);

        if (kill (pid, signal) == -1)
        {
            perror ("kill");
            return;
        }
    }
    dmess ("at end of send_signals");
}

struct sigaction *create_handler (int signum, void (*handler)(int))
{
    struct sigaction *action = new (struct sigaction);

    action->sa_handler = handler;
/*
**  SA_NOCLDSTOP
**  If  signum  is  SIGCHLD, do not receive notification when
**  child processes stop (i.e., when child processes  receive
**  one of SIGSTOP, SIGTSTP, SIGTTIN or SIGTTOU).
*/
    if (signum == SIGCHLD)
    {
        action->sa_flags = SA_NOCLDSTOP;
    }
    else
    {
        action->sa_flags = 0;
    }

    sigemptyset (&(action->sa_mask));

    assert (sigaction (signum, action, NULL) == 0);
    return (action);
}

PCB* choose_process ()
{
    /*
    3) When a SIGALRM arrives, scheduler() will be called. It calls
        choose_process which currently always returns the idle process. Do the
        following.
        a) Update the PCB for the process that was interrupted including the
            number of context switches and interrupts it had, and changing its
            state from RUNNING to READY.
        b) If there are any processes on the new_list, do the following.
            i) Take the one off the new_list and put it on the processes list.
            ii) Change its state to RUNNING, and fork() and execl() it.
        c) Modify choose_process to round robin the processes in the processes
            queue that are READY. If no process is READY in the queue, execute
            the idle process.
    */
    running->interrupts++;
    running->state = READY;

    if (!new_list.empty()) {
        PCB *new_process = new_list.front();

        pid_t pid = fork();
        // only execl if in the child
        if (pid == 0)
        {
            cout << "Executing " << new_process->name;
            if (execl(new_process->name, new_process->name, NULL) == -1 )
            {
                cout  << " failed. Removing from new_list.";
                perror("execl");

                new_list.pop_front();
            }
            cout << "." << endl;
        }
        else if (pid < 0)
        {
            perror("fork");
        }
        else
        {
            processes.push_back(new_process);
            new_list.pop_front();

            new_process->switches++;
            new_process->started = sys_time;
            new_process->pid = pid;
            new_process->ppid = getpid();

            running = new_process;
            running->state = RUNNING;

            return running;
        }
    }
    else if (!processes.empty())
    {
        list<PCB *> :: iterator process = processes.begin();

        for (; process != processes.end(); process++)
        {
            if ((*process)->state == READY)
            {
                if ((*process)->pid != running->pid)
                {
                    running->switches++;
                }

                processes.push_back(*process);
                processes.erase(process);

                // update the state
                running = *process;
                (*process)->state = RUNNING;

                return running;
            }

            running = idle;
        }
    }

    return idle;
}

void scheduler (int signum)
{
    assert (signum == SIGALRM);
    sys_time++;

    PCB* tocont = choose_process();

    dprintt ("continuing", tocont->pid);
    if (kill (tocont->pid, SIGCONT) == -1)
    {
        perror ("kill");
        return;
    }
}

void process_done (int signum)
{
    /*
    4) When a SIGCHLD arrives notifying that a child has exited, process_done() is
        called. process_done() currently only prints out the PID and the status.
        a) Add the printing of the information in the PCB including the number
            of times it was interrupted, the number of times it was context
            switched (this may be fewer than the interrupts if a process
            becomes the only non-idle process in the ready queue), and the total
            system time the process took.
        b) Change the state to TERMINATED.
        c) Start the idle process to use the rest of the time slice.
    */
    assert (signum == SIGCHLD);

    int status, cpid;

    cpid = waitpid (-1, &status, WNOHANG);

    dprintt ("in process_done", cpid);
    cout << running << endl;
    running->state = TERMINATED;
    running = idle;

    if  (cpid == -1)
    {
        perror ("waitpid");
    }
    else if (cpid == 0)
    {
        if (errno == EINTR) { return; }
        perror ("no children");
    }
    else
    {
        dprint (WEXITSTATUS (status));
    }
}

/*
** stop the running process and index into the ISV to call the ISR
*/
void ISR (int signum)
{
    if (kill (running->pid, SIGSTOP) == -1)
    {
        perror ("kill");
        return;
    }
    dprintt ("stopped", running->pid);

    ISV[signum](signum);
}

/*
** set up the "hardware"
*/
void boot (int pid)
{
    ISV[SIGALRM] = scheduler;       create_handler (SIGALRM, ISR);
    ISV[SIGCHLD] = process_done;    create_handler (SIGCHLD, ISR);
    // Z: Probably shouldn't have to create more handlers.

    // start up clock interrupt
    int ret;
    if ((ret = fork ()) == 0)
    {
        // signal this process once a second for NUM_SECOND seconds.
        send_signals (SIGALRM, pid, 1, NUM_SECONDS);

        // once that's done, really kill everything...
        kill (0, SIGTERM);
    }

    if (ret < 0)
    {
        perror ("fork");
    }
}

void create_idle ()
{
    int idlepid;

    if ((idlepid = fork ()) == 0)
    {
        dprintt ("idle", getpid ());

        // the pause might be interrupted, so we need to
        // repeat it forever.
        for (;;)
        {
            dmess ("going to sleep");
            pause ();
            if (errno == EINTR)
            {
                dmess ("waking up");
                continue;
            }
            perror ("pause");
        }
    }
    idle = new (PCB);
    idle->state = RUNNING;
    idle->name = "IDLE";
    idle->pid = idlepid;
    idle->ppid = 0;
    idle->interrupts = 0;
    idle->switches = 0;
    idle->started = sys_time;
}

int main (int argc, char **argv)
{
    int pid = getpid();
    dprintt ("main", pid);

    /*
    2) Take any number of arguments for executables, and place each on new_list.
        The executable will not require arguments themselves.
    */
    if (argc > 0) {
        PCB *arg;

        for (int a = 1; a < argc; a++) {
            arg = new (PCB);
            arg->state = NEW;
            arg->name = argv[a];
            arg->pid = 0;
            arg->ppid = 0;
            arg->interrupts = 0;
            arg->switches = 0;
            arg->started = 0;

            new_list.push_back(arg);
        }
    }
    /* End my section */

    sys_time = 0;

    boot (pid);

    // create a process to soak up cycles
    create_idle ();
    running = idle;

    cout << running;

    // we keep this process around so that the children don't die and
    // to keep the IRQs in place.
    for (;;)
    {
        pause();
        if (errno == EINTR) { continue; }
        perror ("pause");
    }
}

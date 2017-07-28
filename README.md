Add kernel calls to your VirtuOS project.
Do this by creating a pair of pipes to every child process (in each PCB).
A kernel call is made by putting a request in the pipe from the child to the kernel and then sending the kernel a SIGTRAP.
You'll need to create a SIGTRAP ISR that reads the request and sends back a response.
Implement at least the listing of the names of all the current processes and the system time, and implement a child process that requests both.

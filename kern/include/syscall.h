#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int
sys_reboot(int code);

int
sys_write(int fd, const char *buf, size_t bufferLength, int *retval);

void
sys__exit(int);

int
sys__time(time_t *sec, unsigned long *nanosec, int *retval);

int
sys_read(int fd, char *buf, size_t buflen, int *retval);

unsigned int
sys_sleep(unsigned int seconds);

int
sys_fork(struct trapframe *tf, int *retval);

// returns the process id whose exit status is reported in status
// wait for the process specified by pid to exit
// return encoded exit status in the int pointed to by status
pid_t
sys_waitpid(pid_t pid, int* status, int options);

#endif /* _SYSCALL_H_ */

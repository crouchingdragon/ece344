#ifndef SYSCALL_H
#define SYSCALL_H

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
int sys_write(int fd, const char *buf, size_t bufferLength, int *retval);
void sys__exit(int);
int sys__time(time_t *sec, unsigned long *nanosec, int *retval);
int sys_read(int fd, char *buf, size_t buflen, int *retval);
unsigned int sys_sleep(unsigned int seconds);
int sys_fork(struct trapframe *tf, int *retval);
int sys_getpid(int *retval);
int sys_waitpid(pid_t pid, int* status, int options, int *retval);
int sys_execv(const char *program, char **args);
int sys_sbrk(int ammount, int* retval);

#endif /* SYSCALL_H */

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);

/*
 * First open a file
 * check copyout in lib.h
 */
 // call specifies the file name to be opened (file descriptor), a code for open for reading, writing, etc
int
sys_read(int fd, char*  buf, size_t buflen, int *retval);
void
sys__exit(int);
int
sys___time(time_t *sec, unsigned long *nanosec, int *retval);
unsigned int
sys_sleep(unsigned int seconds);
int
sys_write(int fileDest, char *buf, size_t size, int *retval);

// sys_sleep    Makes the calling thread sleep until seconds have elapsed or a signal arrives which is not ignored
//              Returns 0 once time has elapsed
//              No errors


#endif /* _SYSCALL_H_ */

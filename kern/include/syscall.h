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
sys_read();


#endif /* _SYSCALL_H_ */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <syscall.h>
#include <kern/unistd.h>
#include <vnode.h>
#include <uio.h>
#include <vfs.h>
#include <curthread.h>
#include <thread.h>
#include <addrspace.h>
#include <kern/limits.h>
#include <test.h>
#include <clock.h>
#include <syscall.h>
// #include <stdio.h>

/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is
 * passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, like an ordinary function call, and the a3 register is
 * also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments,
 * there should be no need to fetch additional arguments from the
 * user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as
 * arguments, they will get passed in pairs of registers, and not
 * necessarily in the way you expect. We recommend you don't do it.
 * (In fact, we recommend you don't use 64-bit quantities at all. See
 * arch/mips/include/types.h.)
 */

void
mips_syscall(struct trapframe *tf)
{
	int callno;
	int32_t retval;
	int err;

	assert(curspl==0);

	callno = tf->tf_v0;

	/*
	 * Initialize retval to 0. Many of the system calls don't
	 * really return a value, just 0 for success and -1 on
	 * error. Since retval is the value returned on success,
	 * initialize it to 0 by default; thus it's not necessary to
	 * deal with it except for calls that return other values, 
	 * like write.
	 */

	retval = 0;

	switch (callno) {
	    case SYS_reboot:
		err = sys_reboot(tf->tf_a0);
		break;

	    /* Add stuff here */
		case SYS_read:
		err = sys_read(tf->tf_a0, (char*) tf->tf_a1, tf->tf_a2, &retval);
		break;

		case SYS_write:
		err = sys_write(tf->tf_a0, (const char *) tf->tf_a1, tf->tf_a2, &retval);
		break;
		
		case SYS__exit:
		sys__exit(SYS__exit);
		break;

		case SYS___time:
		err = sys__time((time_t *)tf->tf_a0, (unsigned long *) tf->tf_a1, &retval);
		break;

		case SYS_read:
		err = sys_read(tf->tf_a0, (char *) tf->tf_a1, tf->tf_a2, &retval);
		break;

		case SYS_sleep:
        err = sys_sleep((unsigned int) tf->tf_a0);
        break;

		case SYS_fork:
        err = sys_fork(tf, &retval);
        break;
 
	    default:
		kprintf("Unknown syscall %d\n", callno);
		err = ENOSYS;
		break;
	}


	if (err) {
		/*
		 * Return the error code. This gets converted at
		 * userlevel to a return value of -1 and the error
		 * code in errno.
		 */
		tf->tf_v0 = err;
		tf->tf_a3 = 1;      /* signal an error */
	}
	else {
		/* Success. */
		tf->tf_v0 = retval;
		tf->tf_a3 = 0;      /* signal no error */
	}
	
	/*
	 * Now, advance the program counter, to avoid restarting
	 * the syscall over and over again.
	 */
	
	tf->tf_epc += 4;

	/* Make sure the syscall code didn't forget to lower spl */
	assert(curspl==0);
}

void
md_forkentry(void* trap, unsigned long addr)
{
	/*
	 * This function is provided as a reminder. You need to write
	 * both it and the code that calls it.
	 *
	 * Thus, you can trash it and do things another way if you prefer.
	 */

	struct trapframe tf;
    memcpy(&tf,(struct trapframe*) trap,sizeof(struct trapframe));
    kfree(trap);

    // assign address space
    curthread->t_vmspace = (struct addrspace*)addr;
    as_activate(curthread->t_vmspace);

    tf.tf_v0 = 0;
    tf.tf_a3 = 0;
    tf.tf_epc += 4;// increment pc

    mips_usermode(&tf);


	//(void)tf;
}


unsigned int
sys_sleep(unsigned int seconds){
    clocksleep(seconds);
    return 0;
}

int
sys__time(time_t *sec, unsigned long *nanosec, int *retval){
	
	// declare destination variables
	
	time_t *seconds = kmalloc(sizeof(time_t));
	unsigned long *nanoseconds = kmalloc(sizeof(unsigned long));

	//checking for error bad memory access

	if(nanosec != NULL){
		if(copyin((const_userptr_t) nanosec, nanoseconds, sizeof(unsigned long)))
			return EFAULT;
	}

	if(sec != NULL){
		if(copyin((const_userptr_t) sec, seconds, sizeof(time_t)))
			return EFAULT;
	}

	gettime(seconds,(u_int32_t *) nanoseconds);
	copyout(seconds, (userptr_t) sec, sizeof(time_t));
	copyout(nanoseconds, (userptr_t) nanosec, sizeof (unsigned long));

	*retval = *seconds;
	

	kfree(seconds);
	kfree(nanoseconds);
	return 0;
}

void
sys__exit(int exitcode){
	exitcode = 1;
	thread_exit();
	return;
}

int
sys_read(int filedest, char *buf, size_t size, int *retval) {

    //Check EBADF

    if (filedest != STDIN_FILENO) {
        *retval =-1;
		return EBADF;
    }

    //Check EFAULT

    char *momo = kmalloc(size);


    if (copyin((const_userptr_t) buf, momo, size)) {
        kfree(momo);
		*retval = -1;
        return EFAULT;
    }

	if (size != 1){
		*retval = -1;
		return EUNIMP;
	}
    

	struct uio u;
    struct vnode *v;
 	char *con = kstrdup("con:");
	
   
    vfs_open(con, O_RDONLY, &v);
    kfree(con);

   
    mk_kuio(&u, momo, size, 0, UIO_READ);



    int check = VOP_READ(v, &u);

    
    if (check) {

      
	    vfs_close(v);
      
	    kfree(momo);
      
	    return check;   
	}

    
    copyout(momo, (userptr_t) buf, u.uio_offset);
    
	
	kfree(momo);


    vfs_close(v);

   
    *retval = u.uio_offset;
    return 0;
}

int
sys_write(int filedest, const char *buf, size_t size, int *retval) {

    //Check EBADF
    if (filedest != STDOUT_FILENO) {
		if(filedest != STDERR_FILENO)
        	return EBADF;
    }

   

    char *momo = kmalloc(size);
    
	
	
	if (copyin((const_userptr_t) buf, momo, size)) {
        kfree(momo);
        return EFAULT;
    
	
	}

    struct uio u;
    struct vnode *v;
    
    char *con = kstrdup("con:");
   
   
    vfs_open(con, O_WRONLY, &v);
   
    kfree(con);

 
    mk_kuio(&u, momo, size, 0, UIO_WRITE);
 
    int spl = splhigh();
   
    int check = VOP_WRITE(v, &u);
  
  
    splx(spl);

    if (check) {


        vfs_close(v);
        kfree(momo);
	    return check;
   
    }

    vfs_close(v);

    kfree(momo);
    *retval = size;
    return 0;
}

int sys_fork(struct trapframe *tf, int *retval){

	int spl = splhigh();
	
	struct trapframe *duplicate = kmalloc(sizeof(struct trapframe));

	//check memory availibility

	if (duplicate == NULL){
		splx(spl);
		return ENOMEM;
	}

//copy tf mem to duplicate
	memcpy(duplicate, tf, sizeof(struct trapframe));
	
	struct addrspace *adsp;
	struct thread *rope;

	int check = as_copy(curthread->t_vmspace, &adsp);

	if(check != 0){
	
		kfree(duplicate);
		splx(spl);
		return ENOMEM;
	
	}
	//actual forking
	int forkcheck = thread_fork(curthread->t_name, duplicate, (unsigned long) adsp, md_forkentry, &rope);
	
	
	if(forkcheck != 0){

		kfree(duplicate);
		splx(spl);
		return ENOMEM;

	}

	*retval = rope->t_pid;
	splx(spl);
	return 0;
}

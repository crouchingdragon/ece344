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
		case SYS_write:
		err = sys_write(tf->tf_a0, (const char *) tf->tf_a1, tf->tf_a2, &retval);
		break;
		
		case SYS__exit:
		sys__exit((int)tf->tf_a0);
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

		case SYS_getpid:
        err = sys_getpid(&retval);
        break;

        case SYS_waitpid:
        err = sys_waitpid((pid_t)tf->tf_a0, (int*)tf->tf_a1, tf->tf_a2, &retval);
        break;

		case SYS_execv:
		err = sys_execv((const char *)tf->tf_a0, (char**)tf->tf_a1);
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

// int
// sys_getpid(int *retval) {
//     *retval = curthread->pid;
//     return 0;
// }

// void
// sys__exit(int exitcode) {
//     *curthread->exitcode = exitcode;
//     thread_exit(); //thread_detach called in thread_exit
// }

// int sys_write(int fileDest, const void* buf, size_t size, int *retval){

// 	if (fileDest != STDOUT_FILENO && fileDest != STDERR_FILENO)
// 		return EBADF;

// 	//check memory fault
	
// 	if(buf == NULL){
// 		*retval = -1;
// 		return EFAULT;
// 	}	
	
// 	int spl = splhigh();
// 	//checking for bad file number
// 	char *momo = kmalloc((size+1)*sizeof(char));
// 	int check = copyin((const_userptr_t) buf, momo, size);

// 	//open  stdin
// 	// struct uio con;
// 	// struct vnode *vnd;
// 	// char *in = kstrdup("con:"); //copied from waterloo site no idea why or wat it is

//     // vfs_open(in, O_WRONLY, &vnd);
//     // kfree(in); // wtf? idk if i shd do this im scared

// 	// //copy stdin to momo or starting point to write 
// 	// mk_kuio(&con, momo, size, 0, UIO_WRITE);
	
// 	//int check = VOP_WRITE(vnd, &con);
// 	//splx(spl);

// 	//checking if you are writting ?
// 	if(check != 0){

// 		//vfs_close(vnd);
// 		kfree(momo);
// 		*retval = -1;

// 		return EFAULT;
// 	}
// 	else{
// 		momo[size] = '\0';
//         kprintf("%s", momo);
//         kfree(momo);
//         *retval = size;   // successful, return the count of bytes written
//         splx(spl);  // ADD FOR ASST3
//         return 0;
// 	}
// 	// vfs_close(vnd);
// 	// kfree(momo);
// 	// *retval = size;
	
// 	//return 0;

// }


int sys__time(time_t *sec, unsigned long *nanosec, int *retval){
	
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

// void
// sys__exit(int exitcode){
// 	//*curthread->exitcode = exitcode;
//     exitcode = 1;
// 	//thread_exit();
// 	return;
// }

// int
// sys_read(int fd, void *buf, size_t buflen, int *retval) {
 
    
	
	
// 	if (fd != STDIN_FILENO) {
//         *retval = -1;
//         return EBADF;
//     }
	
	
	
// 	//check memory fault
    
	
// 	if (buf == NULL){
//         *retval = -1;
//         return EFAULT;
//     }
	
// 	if (buflen != 1){
// 		*retval = -1;
// 		return EUNIMP;
// 	}
	
	
// 	//char *momo = kmalloc((buflen+1)*sizeof(char));
//     // if(copyin((const_userptr_t) buf, momo, buflen)){
//     //     kfree(momo);
//     //     return EFAULT;
//     // }


// 	// int count  = 0;
// 	// int i;
// 	// for(i=0;i<(int)buflen;i++){
// 	// 	momo[i]= getch();
// 	// 	count++;
// 	// 	if(momo[i] == '\r')
// 	// 		break;
// 	// }
// 	// momo[count] = '\0';
// 	char momo = getch();


//     //kfree(momo);
//     // retval is the count of bytes read by the end of this
//     // on error, return -1 and set erno to specific error code
//     // reads up to buflen bytes from fd, at location specified by current seek position of file, stores them in space pointed to by buf
//     // file must be open for reading
//     // current seek position must be advanced by the number of bytes read
 
//     // check function parameters - is everything legal?
//     // EBADF fd is not a valid file descriptor or was not opened for reading
    
//     // int invalid = copyout(const void *src, buf, buflen); //?
//     // part or all of an address space pointed to by buf is invalid
//     // if (buf == NULL) {
//     //     *retval = -1;
//     //     return EFAULT;
//     // }
//     // length of buffer is not equal to 1
    
 
 
//     // struct uio u;
//     // struct vnode* v; // need to initialize v to something before doing operations
//     int result;
 
   
 
//     // char* con;
//     // con = kstrdup("con: ");
//     // vfs_open(con, O_RDONLY, &v);
//     // kfree(con);
 
//     // if (result){
//     //     vfs_close(v);
//     //     *retval = -1;
//     //     return result;
//     // }
 
//     // initialize uio for read/write - use mk_kuio in uio.c
//     // offset should probably not actually be 0 (should be given by seek position)
//    // mk_kuio(&u, momo, buflen, 0, UIO_READ);
//     result = copyout((const void*)&momo, (userptr_t)buf, buflen);
   
//     if (result != 0){
//       //  vfs_close(v);
//         //*retval = -1;
// 	//	kfree(momo);
// 		*retval = -1;
// 		return EFAULT;
//     }

	
// 		//kfree(momo);
		
// 		*retval = 1;
//         return 0;

	
	




// 	// copyout(momo, (userptr_t) buf, u.uio_offset);
// 	// kfree(momo);
// 	// vfs_close(v);




//    // *retval = u.uio_offset; // how many bytes are left after operation
//     // *retval = 0;
 
//     // int val;
//     // val = getchar();
 
//     //return 0;
// }
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

	*retval = rope->pid;
	splx(spl);
	return 0;
}

int
sys_waitpid(pid_t pid, int* status, int options, int* retval){

   if(options != 0) {
	   *retval = -1;
        return EINVAL;
    }
	if(status == NULL){
		*retval = -1;
        return EFAULT;
    }
	if((unsigned)status == 0x40000000){
		*retval = -1;
        return EFAULT;
    }
	if((unsigned)status == 0x80000000){
		*retval = -1;
        return EFAULT;
    }
	// unaligned status
	if((unsigned)status % 4 != 0){
		*retval = -1;
        return EFAULT;
    }
    if (reap(pid)){
        *retval = -1;
        return EINVAL;
    }
	// waiting on yourself
	if (pid == curthread->pid){
        *retval = -1;
        return EINVAL;
    }
	if (get_parentpid(pid) == -2) {
        *retval = -1;
        return EINVAL;
    }
    if (curthread->pid != get_parentpid(pid)){
        *retval = -1;
        return EINVAL;
    }
	if (pid <= 0){
        *retval = -1;
        return EINVAL;
    }
	if (pid >= 34000){
        *retval = -1;
        return EINVAL;
    }
 
    // if a thread has already exited (zombie), then just return its values
    if (already_exited(pid)){
        *status = get_exitcode(pid);
		freeing_proc(pid);
        *retval = 0;
        return 0;
    }
	// Why does this work?
    while (!already_exited(pid)){
        P_done(pid);
    }

    *status = get_exitcode(pid);
    *retval = 0;
    freeing_proc(pid);

    return 0;
}
 
void
sys__exit(int exitcode){
	// Could use the semaphore 'enter' instead of disabling interrupts
    // int spl;
    // spl = splhigh();

	/* changed to semaphores because asst4 piazza notes said not to rely on interrupts */
	P_enter(curthread->pid);

	// Decrementing count to allow access to P in waitpid
    exit_setting(curthread->pid, exitcode);
    V_done(curthread->pid);

	V_enter(curthread->pid);

    // splx(spl);
    thread_exit();
}
 
int
sys_getpid(int *retval) {
    *retval = (int)curthread->pid;
    return 0;
}

int
sys_execv(const char *prog, char **args){

	if(prog == NULL){
		return EFAULT;
	}
	if((unsigned)prog == 0x40000000){
		return EFAULT;
	}
	if((unsigned)prog == MIPS_KSEG0){
		return EFAULT;
	}
	if(args == NULL){
		return EFAULT;
	}
	if((unsigned)args == 0x40000000){
		return EFAULT;
	}
	if((unsigned)args == MIPS_KSEG0){
		return EFAULT;
	}
	if(*prog == 0){
		return EINVAL;
	}

	int numargs = 0;
	int cnt = 0;
	// counting the nagrs
	while(args[cnt] != NULL){
		if((unsigned)args[cnt] == 0x40000000){
			return EFAULT;
		}
		cnt++;
	}
	numargs = cnt;

	char **temp_args = kmalloc( (numargs + 1) * sizeof(char*));

	int i = 0;

	while (i < numargs){
		int length = strlen(args[i]);
		length++;

		temp_args[i] = (char*) kmalloc(length * sizeof(char));
		if (temp_args[i] == NULL)
			return ENOMEM;

		int result = copyinstr((const_userptr_t) args[i], temp_args[i], PATH_MAX, NULL);
		if (result) 
			return EFAULT;
		

		temp_args[i][length - 1] = '\0';

		i++;
	}

	temp_args[numargs] = NULL;


	size_t size;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;


	char *progname = (char *) kmalloc(sizeof(char) * PATH_MAX);
	if (progname == NULL)
		return ENOMEM;
	int	result = copyinstr((const_userptr_t) prog, progname, PATH_MAX, &size);

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, &v);
	if (result) {
		return result;
	}
	// destroy it
	if (curthread->t_vmspace != NULL) {
		as_destroy(curthread->t_vmspace); 
		curthread->t_vmspace = NULL;

	}

	/* We should be a new thread. */
	assert(curthread->t_vmspace == NULL);

	/* Create a new address space. */
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace == NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);


	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		return result;
	}


	userptr_t ptr[numargs];
	// int i;
    for (i=numargs-1; i >= 0; i--) {
    	
        int len;
        if ( (strlen(temp_args[i])+1) % 4 == 0 ) {
          len = strlen(temp_args[i])+1;
        } else {
           len = (strlen(temp_args[i])+1) + (4-((strlen(temp_args[i])+1)%4));
        }

        stackptr -= len;
        result = copyoutstr(temp_args[i], (userptr_t) stackptr, strlen(temp_args[i])+1, NULL);
        if (result) return result;
        ptr[i] = (userptr_t)stackptr;
    }

	for (i=numargs-1; i >= 0; i--) {
      stackptr -= 4;
      result = copyout(ptr+i, (userptr_t) stackptr, 4);
      if (result) return result;
    }

	kfree(progname);

    md_usermode(numargs,(userptr_t)stackptr, stackptr, entrypoint);

    panic("md_usermode returned\n");
    return EINVAL;

}


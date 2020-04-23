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

#include <vm.h>

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

		case SYS_sbrk:
		err = sys_sbrk((int)tf->tf_a0, &retval);
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
	kprintf("IN MD FORKENTRY\n");
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
	kprintf("In sys_sleep\n");
    clocksleep(seconds);
    return 0;
}

int sys__time(time_t *sec, unsigned long *nanosec, int *retval){
	kprintf("In sys__time\n");
	
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

int
sys_read(int filedest, char *buf, size_t size, int *retval) {
	kprintf("In sys_read\n");

	if (buf == NULL) panic("hey\n");

    //Check EBADF
    if (filedest != STDIN_FILENO) {
        *retval =-1;
		return EBADF;
    }

    //Check EFAULT

    char *momo = kmalloc(size);
	if (momo == NULL) panic("hey m8\n");


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
	
	if (con == NULL) panic("hey m8\n");

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
	// kprintf("In sys_write\n");

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
	kprintf("In sys_fork\n");
	int spl = splhigh();
	// P_enter(curthread->pid); // Changed interrupts to semaphores
	
	struct trapframe *duplicate = kmalloc(sizeof(struct trapframe));

	//check memory availibility
	if (duplicate == NULL){
		splx(spl);
		// V_enter(curthread->pid);
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
		// V_enter(curthread->pid);
		return ENOMEM;
	
	}
	//actual forking
	int forkcheck = thread_fork(curthread->t_name, duplicate, (unsigned long) adsp, md_forkentry, &rope);
	
	
	if(forkcheck != 0){

		kfree(duplicate);
		splx(spl);
		// V_enter(curthread->pid);
		// return ENOMEM;

	}

	*retval = rope->pid;
	splx(spl);
	// V_enter(curthread->pid);
	return 0;
}

int
sys_waitpid(pid_t pid, int* status, int options, int* retval){
	kprintf("In sys_waitpid\n");

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
 
    /* if a thread has already exited (zombie), then just return its values and free it */
    if (already_exited(pid)){
        *status = get_exitcode(pid);
		freeing_proc(pid);
        *retval = 0;
        return 0;
    }
	// Why does this work?
    while (!already_exited(pid)){
        // P_done(pid);
		thread_join(get_who(pid));
		kprintf("WAITS HERE\n");
    }

    *status = get_exitcode(pid);
    *retval = 0;
    freeing_proc(pid);

    return 0;
}
 
void
sys__exit(int exitcode){
	// kprintf("In sys__exit\n");
	// Could use the semaphore 'enter' instead of disabling interrupts
    int spl;
    spl = splhigh();

	/* changed to semaphores because asst4 piazza notes said not to rely on interrupts */
	// P_enter(curthread->pid);

	// Decrementing count to allow access to P in waitpid
    exit_setting(curthread->pid, exitcode);
    // V_done(curthread->pid);

	// V_enter(curthread->pid);
	// cmd_print_coremap();
    splx(spl);
	thread_detach(curthread);
    thread_exit();
	// as_destroy(curthread->t_vmspace);
	// thread_detach(curthread);
}
 
int
sys_getpid(int *retval) {
	kprintf("In sys_getpid\n");
    *retval = (int)curthread->pid;
	// kprintf("pid %d\n", curthread->pid);
    return 0;
}

int
sys_execv(const char *prog, char **args){
	kprintf("In sys_execv\n");

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

	unsigned index;

	for (index = 0; index < sizeof(temp_args); i++){
		// temp_args[i] = NULL;
		kfree(temp_args[i]);
	}
	kfree(temp_args);

	kfree(progname);

    md_usermode(numargs,(userptr_t)stackptr, stackptr, entrypoint);

    panic("md_usermode returned\n");
    return EINVAL;

}

/* ammount: number of bytes of memory to allocate in the heap*/
int
sys_sbrk(int ammount, int* retval){
	kprintf("In sys_sbrk\n");
	// int spl = splhigh();
	// kprintf("ammount hex: 0x%x   int: %d\n", ammount, ammount);

	// defining things
	struct addrspace* as;
	as = curthread->t_vmspace;
	vaddr_t heap_base, heap_top, new_heap_top;
	int rounded_amt;
	heap_base = as->start_heap;
	// heap_top = as->start_heap + as->heap_size * PAGE_SIZE;
	heap_top = as->heap_top;

	unsigned max_pages = 10;
	unsigned heap_pgs;

	// kprintf("start heap: 0x%x   dec: %d\n", heap_base, heap_base);

	// Simplest case: nothing changes, just return
	if (ammount == 0){
		*retval = (int)heap_top;
		// splx(spl);
		return 0;
	}
	// Unaligned ammount: Better to just shoot an error - unaligned number's sign can be ambiguous
	if ((ammount % 4) != 0)
	{
		*retval = -1;
		// splx(spl);
		return EINVAL;
	}
	// if not alligned by a page size, should I round up to a page size? It would probably work fine without
	if ((ammount % PAGE_SIZE) != 0){
		rounded_amt = ROUNDUP(ammount, PAGE_SIZE);
		heap_pgs = rounded_amt / PAGE_SIZE;
	}
	else{
		heap_pgs = ammount / PAGE_SIZE;
	}
	// Either ammount is negative and we decrease heap size, or positive an we extend heap
	if (ammount < 0)
	{
		if (heap_pgs > max_pages){
			*retval = -1;
			// splx(spl);
			return EINVAL;
		}
		// if (curthread->t_vmspace->heap_size > max_pages){
		// 	*retval = -1;
		// 	// splx(spl);
		// 	return EINVAL;
		// }
		kprintf("NEGATIVE\n");
		ammount = ammount * -1;
		new_heap_top = heap_top - ammount;
		kprintf("new heap top hex: 0x%x   dec: %d\n", new_heap_top, new_heap_top);
		
		// Does the new top go past the base of the stack?
		if ((int)new_heap_top < (int)heap_base)
		{
			*retval = -1;
			// splx(spl);
			return EINVAL;
		}
		// Is the region so large that the Coremap could never have held it?
		if (is_there_space(ammount/PAGE_SIZE) == 0)
		{
			*retval = -1;
			// splx(spl);
			return EINVAL;
		}

		// vaddr_t addr = heap_top;
		// int i;
		// while (addr != new_heap_top)
		// {
		// 	kprintf("HERE\n");
		// 	i = get_index(KVADDR_TO_PADDR(addr));
		// 	free_from_core(i);
		// 	addr -= PAGE_SIZE;
		// }

		// Decrease the heap size by the number of pages now de-allocated in the heap
		curthread->t_vmspace->heap_size -= (rounded_amt/PAGE_SIZE);
		curthread->t_vmspace->heap_top = new_heap_top;
		*retval = heap_top;
		// splx(spl);
		return 0;
	}
	// if (curthread->t_vmspace->heap_size > max_pages){
	// 	*retval = -1;
	// 	// splx(spl);
	// 	return ENOMEM;
	// }
	// Are there enough free spaces in the coremap to handle the allocation?
	if (is_there_space(ammount/PAGE_SIZE) == 0){
		*retval = -1;
		// splx(spl);
		return ENOMEM;
	}
	if (heap_pgs > max_pages){
		*retval = -1;
		// splx(spl);
		return EINVAL;
	}
	// Increase the heap size by the number of pages now allocated in the heap
	curthread->t_vmspace->heap_size += (rounded_amt/PAGE_SIZE);
	new_heap_top = heap_top + ammount;
	curthread->t_vmspace->heap_top = new_heap_top;
	kprintf("new heap top: %d\n", new_heap_top);
	kprintf("return value (old heap top): %d\n", heap_top);
	
	*retval = (int)heap_top;
	// splx(spl);
	return 0;
}


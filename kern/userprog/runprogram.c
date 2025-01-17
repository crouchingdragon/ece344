/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, char** args, int nargs)
{
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, &v);
	if (result) {
		return result;
	}

	/* We should be a new thread. */
	assert(curthread->t_vmspace == NULL);

	/* Create a new address space. */
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
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




userptr_t ptr[nargs];
	 int i;
    for (i=nargs-1; i >= 0; i--) {
    	
        int len;
        if ( (strlen(args[i])+1) % 4 == 0 ) {
          len = strlen(args[i])+1;
        } else {
           len = (strlen(args[i])+1) + (4-((strlen(args[i])+1)%4));
        }

        stackptr -= len;
        result = copyoutstr(args[i], (userptr_t) stackptr, strlen(args[i])+1, NULL);
        if (result) return result;
        ptr[i] = (userptr_t )stackptr;
    }

	for (i=nargs-1; i >= 0; i--) {
      stackptr -= 4;
      result = copyout(ptr+i, (userptr_t) stackptr, 4);
      if (result) return result;
    }



	// (void)args;
	// (void)nargs;









	/* Warp to user mode. */
	md_usermode(nargs,(userptr_t)stackptr, stackptr, entrypoint);
	
	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL;
}

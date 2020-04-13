#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

/*
 * alloc_kpages() and free_kpages() are called by kmalloc() and thus the whole
 * kernel will not boot if these 2 functions are not completed.
 */

size_t numofpgs;
size_t staticpgs;

u_int32_t new_start = 0;

int init_vm = 0;
int check = 0;
int found = 0;
 
u_int32_t firstpage_addspace;  
u_int32_t lastpage_addspace;

/* For checking what's in the Coremap*/
void
cmd_print_coremap(void){
	u_int32_t i;
	for (i = 0; i < numofpgs; i++){
		kprintf("\nindex: %d     p_mem: %d       v_mem: %d      ", i, Coremap[i].phy_addspace, Coremap[i].vir_addspace);
		kprintf("id: %d     last: %d    ", Coremap[i].id, Coremap[i].last);
		kprintf("state: %d\n", Coremap[i].state);
	}
}

void
vm_bootstrap(void)
{
	u_int32_t new_page;
    u_int32_t coremap_size;

    coremap_access = sem_create("coremap_sem", 1);
    //FIXME: need to create the page keys before call to getsize. but not sure how since the number of keys needed is reliant on that
 
    ram_getsize(&firstpage_addspace, &lastpage_addspace);
 
    // total number of available spaces in mem
    numofpgs = (lastpage_addspace - firstpage_addspace)/ PAGE_SIZE;
 
    Coremap = (struct Coremap_struct*)PADDR_TO_KVADDR(firstpage_addspace);
   
    coremap_size = numofpgs * sizeof(struct Coremap_struct);
   
    new_page = firstpage_addspace + coremap_size;
 
    staticpgs = (new_page - firstpage_addspace) / PAGE_SIZE + 1; // QUESTION: How does this give you how many fixed pages you have?
   
    u_int32_t i;
    for (i = 0; i < numofpgs; i++) {
 
        Coremap[i].addspace = NULL;
        Coremap[i].id = i;
        Coremap[i].phy_addspace = firstpage_addspace + PAGE_SIZE * i;
 
        if(i > staticpgs) {
            Coremap[i].vir_addspace = 0xDEADBEEF;
            Coremap[i].state = FREE;
            Coremap[i].last = 0;
        }
        else {
            Coremap[i].vir_addspace = PADDR_TO_KVADDR(Coremap[i].phy_addspace);
            Coremap[i].state = FIXED;
            Coremap[i].last = 1;
            new_start++;
                       
        }
    }
    init_vm = 1;
}

vaddr_t 
alloc_kpages(int npages)
{
	/*
	 * Write this.
	 */
	 
	(void)npages;
	return 0;
}

void 
free_kpages(vaddr_t addr)
{
	/*
	 * Write this.
	 */

	(void)addr;
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	/*
	 * Definitely write this.
	 */

	(void)faulttype;
	(void)faultaddress;
	return 0;
}

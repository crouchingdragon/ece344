
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>
#include <elf.h>

#include <array.h>
#include <synch.h>
#include <curthread.h>
/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */


extern size_t numofpgs;
struct addrspace *
as_create(void) //FIXME: Gets interrupted after the first line and ends up at alloc_pages (where I set the next breakpoint) when I hit n
{
    struct addrspace *as = kmalloc(sizeof(struct addrspace));
    if (as==NULL) {
        return NULL;
    }
 
    /*
     * Initialize as needed.
     */
    // as->as_regions = array_create();
    // as->start_heap = 0;
    // as->end_heap = 0;
    // not sure if stack needs to be initialized
    as->start_heap = 0;
    as->heap_size = 0;
    as->stack = 0;
    as->stack_size = 0;
    as->code = 0;
    as->code_size = 0;
    as->data = 0;
    as->data_size = 0;
    as->perm = 0;
    as->old_perm = 0;

    // as->adr_access = lock_create("address space lock");
   
   // do you have to initialize the page tables attatched to it?
   // why can't you use malloc here?
    int i;
    for (i = 0; i < PT_SIZE; i++){
        as->as_ptes[i] = NULL; // first layer of PTE points to nothing
    }
 
    return as;
}
 
int
as_copy(struct addrspace *old, struct addrspace **ret)
{
    int spl = splhigh();
    struct addrspace *newas;
 
    newas = as_create();
    if (newas==NULL) {
        panic("out of memory in as_copy");
        return ENOMEM;
    }

    // save current regions
    newas->start_heap = old->start_heap;
    newas->heap_size = old->heap_size;
    newas->stack = old->stack;
    newas->stack_size = old->stack_size;
    newas->code = old->code;
    newas->code_size = old->code_size;
    newas->data = old->data;
    newas->data_size = old->data_size;
    newas->perm = old->perm;
    newas->old_perm = old->old_perm;
    newas->adr_access = old->adr_access;


 
    //COPYING REGIONS**************************************************************************
 
    // int i;
    // for (i = 0; i < array_getnum(old->as_regions); i++) {
    //     struct as_region* copy = kmalloc(sizeof(struct as_region));
    //     *copy = *((struct as_region*)array_getguy(old->as_regions, i));// not sure if these too lines fully copy stuff
    //     array_add(newas->as_regions, copy);
    // }
 
    //COPYING HEAP*********************************************************************************
 
    // newas->start_heap = old->start_heap;
    // newas->end_heap = old->end_heap;
    // newas->permissions = old->permissions;
//is there anything im missing??? right now im copying the last 12 bits.. 1 (startbit) + 1(endbit) + 10(permission) = 12
 
    //COPYING PAGE TABLE ENTRIES PTE*********************************************************************
 
    int j;
    for (j = 0; j < 1024; j++) {
        if(old->as_ptes[j] != NULL) {
            newas->as_ptes[j] = (struct as_pagetable*) kmalloc(sizeof(struct as_pagetable));
            struct as_pagetable *src = old->as_ptes[j]; // was i before
            struct as_pagetable *copying = newas->as_ptes[j]; //was i before
            int k;
            for (k = 0; k < 1024; k++) {
                copying->PTE[k] = 0;
                if(src->PTE[k] & 0x00000800) { 
                    paddr_t src_paddr = (src->PTE[k] & PAGE_FRAME);
                    vaddr_t dest_vaddr = (j << 22) + (k << 12);
                    paddr_t dest_paddr = alloc_page_userspace(newas, dest_vaddr);// this function is at the bottom but i havent defined it yet as idk where i shd do it
                    memmove((void *) PADDR_TO_KVADDR(dest_paddr),
                    (const void*) PADDR_TO_KVADDR(src_paddr), PAGE_SIZE);
                    copying->PTE[k] |= dest_paddr;
                    copying->PTE[k] |= 0x00000800;
                }
                else{
                    copying->PTE[k] = 0;
                }
            }
        }
        else{
            newas->as_ptes[j] = NULL;
        }
    }
               
    //(void)old;
   
    *ret = newas;
    splx(spl);
    return 0;
}
 
 
// paddr_t alloc_page_userspace(struct addrspace * as, vaddr_t vir_as) {
 
//     int id;
//     unsigned i;
//     for (i = 0; i < numofpgs; i++) {
//         if (Coremap[i].state == 0){
//             id = i;
//             break;
//         }
//     }
//     if(as == NULL)
//         Coremap[id].addspace = curthread->t_vmspace;
//     else
//         Coremap[id].addspace = as;
 
//     Coremap[id].state = 2;
//     Coremap[id].vir_addspace = vir_as;
//     Coremap[id].last = 1;
 
//     return Coremap[id].phy_addspace;
// }
 
 
void
as_destroy(struct addrspace *as)
{
    /*
     * Clean up as needed.
     */
    int spl = splhigh();

    // setting all regions to 0
    as->start_heap = 0;
    as->heap_size = 0;
    as->stack = 0;
    as->stack_size = 0;
    as->code = 0;
    as->code_size = 0;
    as->data = 0;
    as->data_size = 0;
    as->perm = 0;
    as->old_perm = 0;

    unsigned i;
    for (i = 0; i < numofpgs; i++) {
        if(Coremap[i].state != FREE && Coremap[i].addspace == as){ // this was checking that state != 0 before (but it would always not equal 0)
            Coremap[i].addspace = NULL;
            Coremap[i].vir_addspace = 0;
            Coremap[i].state = 0;
            Coremap[i].last = 0;
        }
    }
 
    // array_destroy(as->as_regions);
   
    for(i = 0; i < 1024; i++) {
        if(as->as_ptes[i] != NULL)
            kfree(as->as_ptes[i]);
    }
   
    kfree(as);
    splx(spl);
}

// from dumb vm
void
as_activate(struct addrspace *as)
{
    /*
     * Write this.
     */

	int i, spl;

	(void)as;

	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
 
    // (void)as;  // suppress warning until code gets written
}
 
/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
 /* As define region allocates a vm_object for a particular virtual range, checking for (and rejecting) overlapping regions*/
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
         int readable, int writeable, int executable)
{
    /*
     * Write this.
     */
    // Data, Heap, Stack = READ and WRITE but Code region is READ only
    // vaddr: where region should start, sz: size of the region

    // when you set up code region, do it as read only, therefore later on, 

    size_t npages; 

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME; // 0x00000fff; // lower 12 bits of vaddr == vaddr % PAGE_SIZE -- aligns vaddr to next lowest page addr without affecting upper end
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME; // extends upper end of the range

	npages = sz / PAGE_SIZE; // 

    // all regions are getting the same permissions, so I don't think its necessary to have multiple entries
    // assert(as->code == 0);
    // assert(as->data ==0);
    // as->code = vaddr;
    if((as->code == 0) && (as->code_size == 0)){
        as->code = vaddr;
        as->code_size = npages;
        // as->perm = 0 | (readable << 2) | (writeable << 1) | (executable);
        as->perm = readable | writeable | executable;
        as->start_heap = vaddr + sz;
        as->heap_size = 0;
        if (as->start_heap == 0) panic("heap is not allocating\n");
        return 0;
    }
    if((as->data == 0) && (as->data_size == 0)){
        as->data = vaddr;
        as->data_size = npages;
        as->perm = 0 | (readable << 2) | (writeable << 1) | (executable);
        as->perm = readable | writeable | executable;
        as->start_heap = vaddr + sz;
        as->heap_size = 0; // number of pages in heap
        if (as->start_heap == 0) panic("heap is not allocating\n");
        return 0;
    }
    panic("Didn't define regions");
    return 0;

    // defining heap start and end
  //  as->start_heap = vaddr + sz;
   // as->end_heap = as->start_heap;

    // allocate pages in the coremap to accomodate size of new region (don't need to be contiguous)
    // P(coremap_access);
    // // int i;
	// u_int32_t count, j;
	// count = 0;
    // for (j = 0; j < numofpgs; j++){
    //     if ((Coremap[j].state & FREE) && !(Coremap[j].state & DIRTY)){
    //         Coremap[j].vir_addspace = vaddr; // is this what should be assigned?
    //         Coremap[j].state = Coremap[j].state | DIRTY;
    //         count++;
    //     }
    //     if (count >= npages) break;
    // }
    // V(coremap_access);
    // Mark pages as allocated, update flags, update v_base, v_offset, permissions
    // assert(as->as_regions != NULL);
//struct as_region *current;



// mohit 
	// struct as_region *current = kmalloc(sizeof(struct as_region));
	// current->bottom_vm = vaddr;
	// current->npgs = npages;
	// current->region_permis = 0;
	// current->region_permis = (readable | writeable | executable);
	// array_add(as->as_regions, current);

	// if(array_getnum(as->as_regions) == 2){
	// 	as->start_heap = vaddr + sz;
    // 	as->end_heap = as->start_heap;
	// }

	// return 0;

//




    // for(i = 0; i < array_getnum(as->as_regions); i++){
	// 	current = array_getguy(as->as_regions, i);
    //     if (current->npgs == 0) break; // gets next empty region in array
    // }
    // current->bottom_vm = vaddr + sz - 1; // top vm is vaddr
    // current->npgs = npages;
    // current->region_permis = readable | writeable | executable;
    // current->old_perm = -1; // no old


	// array_setguy(as->as_regions, i, current);

    // set up corresponding pte's for each page allocated
    // for (i = 0; i < npages; i++){
    //     as->as_ptes[i] = (struct as_pagetable*) kmalloc(sizeof(struct as_pagetable));

    // }

    // (void)as;
    // (void)vaddr;
    // (void)sz;
    // /* We don't use these - all pages are read-write */
    // (void)readable;
    // (void)writeable;
    // (void)executable;
    // return EUNIMP;
   // return 0;
}
 
int
as_prepare_load(struct addrspace *as)
{
    /*
     * Write this.
     */
    as->old_perm = as->perm;
    as->perm = PF_W | PF_R;
    // change each regions page table permission to read-write since we're going to load content (code, data) into them
    // sets all permissions to write
    // you should not only overwrite the permission, but keep track of what it was before you overwrote it (as complete load restores this)
    // int region_size = array_getnum(as->as_regions);
    // int i;
	// struct as_region *current;
    // for (i = 0; i < region_size; i++){
	// 	current = array_getguy(as->as_regions, i);
	// 	current->old_perm = current->region_permis;
	// 	current->region_permis = READ_WRITE; //FIXME: Not sure if this is the right read and write flag
	// 	array_setguy(as->as_regions, i, current);
        // as->as_regions[i].old_perm = as->as_regions[i].region_permis;
        // as->as_regions[i].region_permis = READ | WRITE;
    // }
    // as->old_perm = as->permissions;
    // as->permissions = READ | WRITE;
 
    // (void)as;
    return 0;
}
 
int
as_complete_load(struct addrspace *as)
{
    /*
     * Write this.
     */
    as->perm = as->old_perm;
    as->old_perm = 0;
    // int region_size = array_getnum(as->as_regions);
    // int i;
	// struct as_region *current;
    // for (i = 0; i < region_size; i++){
	// 	current = array_getguy(as->as_regions, i);
	// 	current->region_permis = current->old_perm;
	// 	array_setguy(as->as_regions, i, current);
        // as->as_regions[i].region_permis = as->as_regions[i].old_perm;
    // }
    // as->permissions = as->old_perm;
    // as->old_perm = -1; // or something undefined

    // (void)as;
    return 0;
}
 
int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
    /*
     * Write this.
     */
 
    // assert(as->as_stackpbase != 0);
    (void)as;
 
    /* Initial user-level stack pointer */
    *stackptr = USERSTACK;
   
    return 0;
}

////////////////////// DUMB VM /////////////////////////////////////

// int
// vm_fault(int faulttype, vaddr_t faultaddress)
// {
// 	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
// 	paddr_t paddr;
// 	int i;
// 	u_int32_t ehi, elo;
// 	struct addrspace *as;
// 	int spl;

// 	spl = splhigh();

// 	faultaddress &= PAGE_FRAME;

// 	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

// 	switch (faulttype) {
// 	    case VM_FAULT_READONLY:
// 		/* We always create pages read-write, so we can't get this */
// 		panic("dumbvm: got VM_FAULT_READONLY\n");
// 	    case VM_FAULT_READ:
// 	    case VM_FAULT_WRITE:
// 		break;
// 	    default:
// 		splx(spl);
// 		return EINVAL;
// 	}

// 	as = curthread->t_vmspace;
// 	if (as == NULL) {
// 		/*
// 		 * No address space set up. This is probably a kernel
// 		 * fault early in boot. Return EFAULT so as to panic
// 		 * instead of getting into an infinite faulting loop.
// 		 */
// 		return EFAULT;
// 	}

// 	/* Assert that the address space has been set up properly. */
// 	assert(as->as_vbase1 != 0);
// 	assert(as->as_pbase1 != 0);
// 	assert(as->as_npages1 != 0);
// 	assert(as->as_vbase2 != 0);
// 	assert(as->as_pbase2 != 0);
// 	assert(as->as_npages2 != 0);
// 	assert(as->as_stackpbase != 0);
// 	assert((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
// 	assert((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
// 	assert((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
// 	assert((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
// 	assert((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

// 	vbase1 = as->as_vbase1;
// 	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
// 	vbase2 = as->as_vbase2;
// 	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
// 	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
// 	stacktop = USERSTACK;

// 	if (faultaddress >= vbase1 && faultaddress < vtop1) {
// 		paddr = (faultaddress - vbase1) + as->as_pbase1;
// 	}
// 	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
// 		paddr = (faultaddress - vbase2) + as->as_pbase2;
// 	}
// 	else if (faultaddress >= stackbase && faultaddress < stacktop) {
// 		paddr = (faultaddress - stackbase) + as->as_stackpbase;
// 	}
// 	else {
// 		splx(spl);
// 		return EFAULT;
// 	}

// 	/* make sure it's page-aligned */
// 	assert((paddr & PAGE_FRAME)==paddr);

// 	for (i=0; i<NUM_TLB; i++) {
// 		TLB_Read(&ehi, &elo, i);
// 		if (elo & TLBLO_VALID) {
// 			continue;
// 		}
// 		ehi = faultaddress;
// 		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
// 		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
// 		TLB_Write(ehi, elo, i);
// 		splx(spl);
// 		return 0;
// 	}

// 	kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
// 	splx(spl);
// 	return EFAULT;
// }

// struct addrspace *
// as_create(void)
// {
// 	struct addrspace *as = kmalloc(sizeof(struct addrspace));
// 	if (as==NULL) {
// 		return NULL;
// 	}

// 	as->as_vbase1 = 0;
// 	as->as_pbase1 = 0;
// 	as->as_npages1 = 0;
// 	as->as_vbase2 = 0;
// 	as->as_pbase2 = 0;
// 	as->as_npages2 = 0;
// 	as->as_stackpbase = 0;

// 	return as;
// }

// void
// as_destroy(struct addrspace *as)
// {
// 	kfree(as);
// }

// void
// as_activate(struct addrspace *as)
// {
// 	int i, spl;

// 	(void)as;

// 	spl = splhigh();

// 	for (i=0; i<NUM_TLB; i++) {
// 		TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
// 	}

// 	splx(spl);
// }

// int
// as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
// 		 int readable, int writeable, int executable)
// {
// 	size_t npages; 

// 	/* Align the region. First, the base... */
// 	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
// 	vaddr &= PAGE_FRAME;

// 	/* ...and now the length. */
// 	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

// 	npages = sz / PAGE_SIZE;

// 	/* We don't use these - all pages are read-write */
// 	(void)readable;
// 	(void)writeable;
// 	(void)executable;

// 	if (as->as_vbase1 == 0) {
// 		as->as_vbase1 = vaddr;
// 		as->as_npages1 = npages;
// 		return 0;
// 	}

// 	if (as->as_vbase2 == 0) {
// 		as->as_vbase2 = vaddr;
// 		as->as_npages2 = npages;
// 		return 0;
// 	}

// 	/*
// 	 * Support for more than two regions is not available.
// 	 */
// 	kprintf("dumbvm: Warning: too many regions\n");
// 	return EUNIMP;
// }

// int
// as_prepare_load(struct addrspace *as)
// {
// 	assert(as->as_pbase1 == 0);
// 	assert(as->as_pbase2 == 0);
// 	assert(as->as_stackpbase == 0);

// 	as->as_pbase1 = getppages(as->as_npages1);
// 	if (as->as_pbase1 == 0) {
// 		return ENOMEM;
// 	}

// 	as->as_pbase2 = getppages(as->as_npages2);
// 	if (as->as_pbase2 == 0) {
// 		return ENOMEM;
// 	}

// 	as->as_stackpbase = getppages(DUMBVM_STACKPAGES);
// 	if (as->as_stackpbase == 0) {
// 		return ENOMEM;
// 	}

// 	return 0;
// }

// int
// as_complete_load(struct addrspace *as)
// {
// 	(void)as;
// 	return 0;
// }

// int
// as_define_stack(struct addrspace *as, vaddr_t *stackptr)
// {
// 	assert(as->as_stackpbase != 0);

// 	*stackptr = USERSTACK;
// 	return 0;
// }

// int
// as_copy(struct addrspace *old, struct addrspace **ret)
// {
// 	struct addrspace *new;

// 	new = as_create();
// 	if (new==NULL) {
// 		return ENOMEM;
// 	}

// 	new->as_vbase1 = old->as_vbase1;
// 	new->as_npages1 = old->as_npages1;
// 	new->as_vbase2 = old->as_vbase2;
// 	new->as_npages2 = old->as_npages2;

// 	if (as_prepare_load(new)) {
// 		as_destroy(new);
// 		return ENOMEM;
// 	}

// 	assert(new->as_pbase1 != 0);
// 	assert(new->as_pbase2 != 0);
// 	assert(new->as_stackpbase != 0);

// 	memmove((void *)PADDR_TO_KVADDR(new->as_pbase1),
// 		(const void *)PADDR_TO_KVADDR(old->as_pbase1),
// 		old->as_npages1*PAGE_SIZE);

// 	memmove((void *)PADDR_TO_KVADDR(new->as_pbase2),
// 		(const void *)PADDR_TO_KVADDR(old->as_pbase2),
// 		old->as_npages2*PAGE_SIZE);

// 	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
// 		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
// 		DUMBVM_STACKPAGES*PAGE_SIZE);
	
// 	*ret = new;
// 	return 0;
// }


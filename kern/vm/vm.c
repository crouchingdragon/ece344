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

static
paddr_t
getppages(unsigned long npages)
{
    int spl;
    paddr_t addr;
    spl = splhigh();
    addr = ram_stealmem(npages);
    splx(spl);
    return addr;
}

// free address?
int is_free(void){
    u_int32_t id;
    for(id = new_start; id < numofpgs; id++){
        if(Coremap[id].state & FREE){
            return id;
        }
    }
    return -1;
}
 
u_int32_t
has_space(int npgs){
    int cont = 1;
    u_int32_t i;
    // numofpgs currently has entire size of coremap, but some of those pages were initialized to fixed in boot
	for (i = new_start; i < numofpgs; i++){
        if(cont >= npgs)
            return (i - npgs + 1); // returns start of block
        if((Coremap[i].state & FREE) && (Coremap[i + 1].state & FREE))
            cont++;
        else
        {
            cont = 1;
        }
        
    }
    return 0;
}

vaddr_t alloc_one_page(void){
    P(coremap_access);
    int free_index = is_free();
	if (!free_index) panic("ran out of memory");
    Coremap[free_index].last = 1; // the last page in the block
    Coremap[free_index].addspace = curthread->t_vmspace;
    Coremap[free_index].state = DIRTY;
    Coremap[free_index].vir_addspace = PADDR_TO_KVADDR(Coremap[free_index].phy_addspace);

	// kprintf("\nAllocated One Page\n");
	// kprintf("index = %d   phys_addr = %d   vir_addr = %d   state = %d  last = %d\n",
	// 			free_index, Coremap[free_index].phy_addspace, Coremap[free_index].vir_addspace,
	// 			Coremap[free_index].state, Coremap[free_index].last);
    // kprintf("\nAllocated 1 page\n");
    V(coremap_access);
	// kprintf("\nAllocated 1 page\n");
	// kprintf("i = %d   Coremap_state: %d  Coremap_last: %d   Coremap vaddr = %d\n",
			// free_index, Coremap[free_index].state, Coremap[free_index].last, Coremap[free_index].vir_addspace);
    return (Coremap[free_index].vir_addspace);
}
 
vaddr_t alloc_pages(int npgs){
	P(coremap_access);
	u_int32_t start = has_space(npgs);
	if (!start) panic("no contiguous space in coremap to allocate pages");
	u_int32_t end = start + npgs;
	u_int32_t i;
	for (i = start; i < end; i++){
		Coremap[i].vir_addspace = PADDR_TO_KVADDR(Coremap[i].phy_addspace);
		Coremap[i].addspace = curthread->t_vmspace;
		Coremap[i].state = DIRTY;
		if (i == (end-1)) Coremap[i].last = 1;
		else Coremap[i].last = 0;
	}
	kprintf("\nAllocated %d pages\n", npgs);
	V(coremap_access);
	return (PADDR_TO_KVADDR(Coremap[start].phy_addspace));
}

 
vaddr_t
alloc_kpages(int npages)
{
    // If we haven't booted, we can safely run stealmem
    if (!init_vm) return PADDR_TO_KVADDR(getppages(npages));
    if (npages == 1) return (alloc_one_page());
    return (alloc_pages(npages));
}

int
index_from_vaddr(vaddr_t addr){
	u_int32_t i;
	for (i = 0; i < numofpgs; i++){
		if (Coremap[i].vir_addspace == addr) return i;
	}
	return -1;
}
 
void
free_kpages(vaddr_t addr)
{
	int int_flag = 0;
	if (!in_interrupt){ // If interrupts have already been disabled and it gets here, no need to use semaphore
		P(coremap_access);
		int_flag = 1;
	}
	// if (!int_flag) kprintf("Interrupts are not blocked\n");
	int i = index_from_vaddr(addr);
	if (i == -1) panic("no virtual address match in coremap");
	while ((Coremap[i].last == 0) && !(Coremap[i].state & FIXED)) {

		// kprintf("ABOUT TO FREE:  i = %d   Coremap_state: %d  Coremap_last: %d   Coremap vaddr = %d\n",
		// 	i, Coremap[i].state, Coremap[i].last, Coremap[i].vir_addspace);

		Coremap[i].state = FREE;

		// kprintf("FREED:  i = %d   Coremap_state: %d  Coremap_last: %d   Coremap vaddr = %d\n",
		// 	i, Coremap[i].state, Coremap[i].last, Coremap[i].vir_addspace);

		i++;
	}
	Coremap[i].state = FREE;
	Coremap[i].last = 0;

	// kprintf("FREED:  i = %d   Coremap_state: %d  Coremap_last: %d   Coremap vaddr = %d\n",
	// 		i, Coremap[i].state, Coremap[i].last, Coremap[i].vir_addspace);

	if (int_flag) V(coremap_access);
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	
	struct addrspace *as;
	int retval;
	u_int32_t permission = 0;
	int spl;

	spl = splhigh();

	faultaddress &= PAGE_FRAME;
	//FIXME: Freaks out and panicks here
	if (faultaddress == 0){ // had == NULL before but vaddr_t is type int
    	splx(spl);
    	return EFAULT;
    }
	if(faulttype == VM_FAULT_READONLY){
		splx(spl);
		return EFAULT;
	}
	if(faulttype != VM_FAULT_READ){
		splx(spl);
		return EINVAL;
	}
	if(faulttype != VM_FAULT_WRITE){
		splx(spl);
		return EINVAL;
	}

	as = curthread->t_vmspace;

	if (as == NULL) {
		/*
		 * No address space set up. This is probably a kernel
		 * fault early in boot. Return EFAULT so as to panic
		 * instead of getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	vaddr_t start_vm, end_vm;

	int i;

	for(i = 0; i < array_getnum(as->as_regions); i++){
		struct as_region *current = array_getguy(as->as_regions, i);
		end_vm = current->bottom_vm;
		start_vm = end_vm + current->npgs * PAGE_SIZE;
		if(faultaddress >= end_vm && faultaddress < start_vm){
			found = 1;
			permission = (current->region_permis);
			retval = faults(faultaddress, permission);
			splx(spl);
			return retval;
		}
	}
//check stack if not found
	if(!found){
		fault_stack(faulttype, faultaddress, &retval);
		if(found){
			splx(spl);
			return retval;
		}
	}
// check heap if not found
	if(!found){
		fault_heap(faulttype, faultaddress, &retval, as);
		if(found){
			splx(spl);
			// return err;
			return EFAULT; // not sure if this should be efault, but err above is undeclared
		}
	}

	splx(spl);
	return EFAULT;
}

void fault_stack(int faulttype, vaddr_t faultaddress, int* retval){
	u_int32_t permissions = 0;
	vaddr_t start_vm, end_vm;
	start_vm = MIPS_KSEG0;
	end_vm = start_vm - 24 * PAGE_SIZE;
	if(faultaddress >= end_vm && faultaddress < start_vm){
		found = 1;
		permissions = 6;
		*retval = faults(faultaddress, permissions);
	}
	(void)faulttype;
}

void fault_heap(int faulttype, vaddr_t faultaddress, int* retval, struct addrspace* as){
	u_int32_t permissions = 0;
	vaddr_t start_vm, end_vm;
	end_vm = as->start_heap;
	start_vm = as->end_heap;
	if(faultaddress >= end_vm && faultaddress < start_vm){
		found = 1;
		permissions = 6;
		*retval = faults(faultaddress, permissions);	
	}
	(void)faulttype;
}

int faults(vaddr_t faultaddress, u_int32_t permissions) {

	int spl = splhigh();
	// vaddr_t vaddr;
	paddr_t paddr;
    u_int32_t tlb_end, tlb_start;

 	//function to see if second level page table exists or not, handles accordingly
 	check_levels(faultaddress, &paddr);

	//load into TLB
	if (permissions & PF_W) {
		paddr |= TLBLO_DIRTY;  
	}

	int k;	
	for(k = 0; k < NUM_TLB; k++){
		TLB_Read(&tlb_end, &tlb_start, k);
		// skip valid ones
		if(tlb_start & TLBLO_VALID){
			continue;
		}
		//fill first empty one
		tlb_end = faultaddress;
		tlb_start = paddr | TLBLO_VALID; 
		TLB_Write(tlb_end, tlb_start, k);
		splx(spl);
		return 0;
	}
	// no invalid ones => pick entry and random and expel it
	tlb_end = faultaddress;
	tlb_start = paddr | TLBLO_VALID;
	TLB_Random(tlb_end, tlb_start);
	splx(spl);
	return 0;
}


void check_levels(vaddr_t faultaddress, paddr_t* paddr){

	int level1_index = (faultaddress & Firstlevel) >> 22; 
	int level2_index = (faultaddress & Secondlevel) >> 12;
	// check if the 2nd level page table exists
	struct as_pagetable *lvl2_ptes = curthread->t_vmspace->as_ptes[level1_index];

	if(lvl2_ptes != NULL) {
	
		u_int32_t *pte = &(lvl2_ptes->PTE[level2_index]);

		if (*pte & 0x00000800) {
			// page is present in physical memory
			*paddr = *pte & PAGE_FRAME; 
		} 
		else {
			 if (*pte) { 
			 	int freed_id = is_free();
				*paddr = load_seg(freed_id, curthread->t_vmspace, faultaddress);

			 } else {
				// page does not exist
				*paddr = alloc_page_userspace(NULL, faultaddress);
				}
			// now update the PTE
			*pte &= 0x00000fff;
			*pte |= *paddr;
	    	*pte |= 0x00000800;
		}
	} else {

		// If second page table doesn't exist, create one
		curthread->t_vmspace->as_ptes[level1_index] = kmalloc(sizeof(struct as_pagetable));
		lvl2_ptes = curthread->t_vmspace->as_ptes[level1_index];

		int i;
		for (i = 0; i < PT_SIZE; i++) {
			lvl2_ptes->PTE[i] = 0;
		}
	    // allocate a page and do the mapping
	    *paddr = alloc_page_userspace(NULL, faultaddress);
		
	    u_int32_t* pte = retEntry(curthread, faultaddress); 

		
		*pte &= 0x00000fff;
	    *pte |= 0x00000800;
	    *pte |= *paddr;
	}
}

paddr_t load_seg(int id, struct addrspace* as, vaddr_t v_as) {

	Coremap[id].state = 2; 
	Coremap[id].addspace = as;
	Coremap[id].vir_addspace = v_as;
	Coremap[id].last = 1;

	return Coremap[id].phy_addspace;
}

u_int32_t* retEntry (struct thread* addrspace_owner, vaddr_t va){

	int level1_index = (va & Firstlevel) >> 22; 
	int level2_index = (va & Secondlevel) >> 12;
	struct as_pagetable* lvl2_ptes = addrspace_owner->t_vmspace->as_ptes[level1_index];

	if (lvl2_ptes == NULL) 
		return NULL;
	else 
		return &(lvl2_ptes->PTE[level2_index]);
}

paddr_t alloc_page_userspace(struct addrspace * as, vaddr_t v_as) {

	int freed_id = is_free();

	if(as == NULL)
		Coremap[freed_id].addspace = curthread->t_vmspace;
	else 
		Coremap[freed_id].addspace = as;

	Coremap[freed_id].state = DIRTY; // was 2 (assuming dirty)
	Coremap[freed_id].vir_addspace = v_as;
	Coremap[freed_id].last = 1;

	return Coremap[freed_id].phy_addspace;
}


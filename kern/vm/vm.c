#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <synch.h>
#include <array.h>
#include <elf.h>
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
int free = 0;
 
u_int32_t firstpage_addspace;  
u_int32_t lastpage_addspace;

int
get_index(paddr_t paddr)
{
	return ((paddr - firstpage_addspace) / PAGE_SIZE);
}

/* For checking what's in the Coremap*/
void
cmd_print_coremap(void){
	u_int32_t i;
	for (i = 0; i < numofpgs; i++){
		kprintf("\nindex: %d     p_mem: %d       v_mem: %d      ", i, Coremap[i].phy_addspace, Coremap[i].vir_addspace);
		kprintf("last: %d    ", Coremap[i].last);
		kprintf("state: %d\n", Coremap[i].state);
	}
}

void
vm_bootstrap(void)
{
	int spl;
	spl = splhigh();

	u_int32_t new_page;
    u_int32_t coremap_size;

    // coremap_access = sem_create("coremap_sem", 1);
	core_lock = lock_create("core lock");
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
        Coremap[i].phy_addspace = firstpage_addspace + PAGE_SIZE * i;
 
        if(i > staticpgs) {
            Coremap[i].vir_addspace = 0xDEADBEEF;
            Coremap[i].state = FREE;
            Coremap[i].last = 0;
			free++;
        }
        else {
            Coremap[i].vir_addspace = PADDR_TO_KVADDR(Coremap[i].phy_addspace);
            Coremap[i].state = FIXED;
            Coremap[i].last = 1;
            new_start++;
			free--;
                       
        }
    }
    init_vm = 1;
	kprintf("FREE PAGES: %d\n", free);
	// cmd_print_coremap();
	splx(spl);
}

static
paddr_t
getppages(unsigned long npages)
{
    int spl;
    paddr_t addr;
    spl = splhigh();
    addr = ram_stealmem(npages);
	// kprintf("in getppages\n");
	// cmd_print_coremap();
    splx(spl);
    return addr;
}

// free address?
int is_free(void){
    u_int32_t id;
    for(id = new_start; id < numofpgs; id++){
        if(Coremap[id].state == FREE){
			// kprintf("free entry: Coremap[%d].state = %d\n", id, Coremap[id].state);
            return id;
        }
    }
    return -1;
}
 
u_int32_t
has_space(int npgs){
    int cont = 1;
    u_int32_t i;
	kprintf("in has space\n");
	// cmd_print_coremap();
    // numofpgs currently has entire size of coremap, but some of those pages were initialized to fixed in boot
	for (i = new_start; i < numofpgs; i++){
        if(cont >= npgs)
			// kprintf("start of the block that has space: %d\n", (i - npgs + 1));
            return (i - npgs + 1); // returns start of block
        if((Coremap[i].state == FREE) && (Coremap[i + 1].state == FREE))
            cont++;
        else
        {
            cont = 1;
        }
        
    }
	kprintf("no space\n");
    return 0;
}

int
is_there_space(int npages)
{
	if (npages > free) return 0;
	return 1;
}

vaddr_t
alloc_user_page(vaddr_t faultaddress)
{
	(void)faultaddress;
	int free_index;
	// int spl = splhigh();
	kprintf("in alloc user page\n");
	// if (free == 0) panic("NEED TO HANDLE PAGING\n");
	// if (free == 0) free_index = free_user_page(faultaddress);
	if (free == 0) return ENOMEM;
	else{
		lock_acquire(core_lock);
		free_index = is_free();
		if (free_index < 0) panic("\nWeird. Something went wrong.\n");
	}
	// if (free_index < 0) free_index = free_user_page();
    Coremap[free_index].last = 1; // the last page in the block
    Coremap[free_index].addspace = curthread->t_vmspace;
    Coremap[free_index].state = DIRTY;
	// Coremap[free_index].vir_addspace = faultaddress;
    Coremap[free_index].vir_addspace = PADDR_TO_KVADDR(Coremap[free_index].phy_addspace);
	free--;
	kprintf("FREE PAGES: %d\n", free);
	// kprintf("Allocated: Coremap[%d]   phys = %d   vir = %d\n", free_index, Coremap[free_index].phy_addspace, Coremap[free_index].vir_addspace);
	lock_release(core_lock);
    // V(coremap_access);
	// splx(spl);
    return (Coremap[free_index].vir_addspace);
}

int
random_index(void)
{
	return ((random() % (numofpgs - new_start)) + new_start);
}

int
free_user_page(vaddr_t faultaddress)
{
	kprintf("Looking randomly for a page to flush\n");
	int index, found, state;
	state = FIXED;
	while (state == FIXED){
		index = random_index();
		state = Coremap[index].state;
	}
	kprintf("Found a page to flush\n");

	assert(index > 0);
	assert((unsigned)index < numofpgs);

	free_from_core(index);

	(void)found;
	(void)faultaddress;

	// tlb handler should do this for us
	// found = TLB_Probe(faultaddress, TLBLO_INVALID());
	// if (found != -1) TLB_Write(TLBHI_INVALID(found), TLBLO_INVALID(), found);
	return index;
}

vaddr_t alloc_one_page(void){
    // P(coremap_access);
	if (free == 0) panic("out of mem\n");
	int free_index;
	// int spl = splhigh();
	lock_acquire(core_lock);
	free_index = is_free();
	kprintf("in alloc one page\n");
	if (free_index < 0) panic("\nran out of memory\n");
    Coremap[free_index].last = 1; // the last page in the block
    Coremap[free_index].addspace = curthread->t_vmspace;
    Coremap[free_index].state = FIXED;
    Coremap[free_index].vir_addspace = PADDR_TO_KVADDR(Coremap[free_index].phy_addspace);
	free--;
	kprintf("FREE PAGES: %d\n", free);
	// kprintf("Allocated: Coremap[%d]   phys = %d   vir = %d\n", free_index, Coremap[free_index].phy_addspace, Coremap[free_index].vir_addspace);
    // V(coremap_access);
	// splx(spl);
	lock_release(core_lock);
    return (Coremap[free_index].vir_addspace);
}
 
vaddr_t alloc_pages(int npgs){
	// P(coremap_access);
	// int spl;
	// spl = splhigh();
	kprintf("in alloc pages\n");
	// cmd_print_coremap();
	lock_acquire(core_lock);
	u_int32_t start = has_space(npgs);
	if (!start) panic("no contiguous space in coremap to allocate pages");
	u_int32_t end = start + npgs;
	u_int32_t i;
	for (i = start; i < end; i++){
		Coremap[i].vir_addspace = PADDR_TO_KVADDR(Coremap[i].phy_addspace);
		Coremap[i].addspace = curthread->t_vmspace;
		Coremap[i].state = FIXED; //FIXME: should be fixed for kernel pages but free for user pages
		if (i == (end-1)) Coremap[i].last = 1;
		else Coremap[i].last = 0;
		free--;
		kprintf("FREE PAGES: %d\n", free);
		// kprintf("Allocated: Coremap[%d].physaddspace = %d\n", i, Coremap[i].phy_addspace);
	}
	lock_release(core_lock);
	// kprintf("\nAllocated %d pages\n", npgs);
	// V(coremap_access);
	// splx(spl);
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

// paddr_t
// alloc_upages(int npages)
// {
//     if (npages == 1) return (alloc_one_page());
//     return (alloc_pages(npages));
// }

int
index_from_vaddr(vaddr_t addr){
	u_int32_t i;
	for (i = 0; i < numofpgs; i++){
		// if (Coremap[i].phy_addspace == KVADDR_TO_PADDR(addr)) return i;
		if (Coremap[i].vir_addspace == addr) return i;
	}
	return -1;
}

int
index_from_paddr(paddr_t addr){
	// return(addr/PAGE_SIZE);
	u_int32_t i;
	for (i = 0; i < numofpgs; i++){
		// if (Coremap[i].phy_addspace == KVADDR_TO_PADDR(addr)) return i;
		if (Coremap[i].phy_addspace == addr) return i;
	}
	return -1;
}

void
free_from_core(int index){
	kprintf("%d\n", Coremap[index].state);
	if (Coremap[index].state != FIXED){
		Coremap[index].state = FREE;
		Coremap[index].addspace = NULL;
		Coremap[index].last = 0;
		free++;
	}
	kprintf("FREE PAGES: %d\n", free);
}

void
kill_proc_map(){
	unsigned i;
	for (i = 0; i < numofpgs; i++){
		if (Coremap[i].state == FIXED) continue;
		Coremap[i].state = FREE;
		Coremap[i].addspace = NULL;
		Coremap[i].vir_addspace = 0;
		Coremap[i].last = 0;
	}
}
 
void
free_kpages(vaddr_t addr)
{
	int int_flag = 0;
	if (!in_interrupt){ // If interrupts have already been disabled and it gets here, no need to use semaphore
		// P(coremap_access);
		lock_acquire(core_lock);
		int_flag = 1;
	}
	// int spl;
	// spl = splhigh();
	// int i = index_from_vaddr(addr);
	int i = get_index(KVADDR_TO_PADDR(addr));
	while (Coremap[i].last != 1) {
		free_from_core(i);
		// kprintf("just freed Coremap[%d], vaddr = %d\n", i, addr);
		i++;
	}
	free_from_core(i);
	kprintf("FREE PAGES: %d\n", free);
	// kprintf("just freed Coremap[%d], vaddr = %d\n", i, addr);
	// cmd_print_coremap();
	// splx(spl);
	if (int_flag){
		// V(coremap_access);
		lock_release(core_lock);
	}
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	// int spl;
	// spl = splhigh();

	faultaddress &= PAGE_FRAME;

	struct addrspace* as;
	as = curthread->t_vmspace;

	// some asserts to check assumptions about alignment
	assert(as->code != 0);
	assert(as->code_size != 0);
	assert(as->data != 0);
	assert(as->data_size != 0);
	assert(as->start_heap != 0);
	// assert(as->heap_size != 0);
	assert((as->code % PAGE_FRAME) == as->code);
	assert((as->data % PAGE_FRAME) == as->data);
	assert((as->start_heap % PAGE_FRAME) == as->start_heap);

	// making sure fault type is what we expect it to be
	if (faulttype == VM_FAULT_READONLY) panic("got VM_FAULT_READONLY");
	if (faulttype != VM_FAULT_READ && faulttype != VM_FAULT_WRITE){
		panic("what fault did I get?\n");
		// splx(spl);
		return EINVAL;
	}

	// make sure the current process has an actual address space here
	if (curthread->t_vmspace == NULL) {
		// panic("process space is null\n");
		kprintf("CURTHREAD VMSPACE IS NULL\n");
		// splx(spl);
		return EFAULT;
	}

	// If the fault address is not in a valid region, it's a bad fault
	if (bad_fault(faultaddress, curthread->t_vmspace)){
		// kprintf("failing on faultaddress %d\n", faultaddress);
		// kprintf("BAD ADDRESS\n");
		// splx(spl);
		return EFAULT;
	}

	if (free == 0){
		// kprintf("NO FREE PAGES\n");
		// splx(spl);
		return ENOMEM;
	}
	// if it's not a bad address, locate the address in the page table
	paddr_t page = get_page(faultaddress, curthread->t_vmspace);
	if (page == ENOMEM){
		kprintf("NO FREE PAGES 2\n");
		// splx(spl);
		return ENOMEM;
	}

	int spl = splhigh();
	// update the TLB entries so that this address won't fault anymore
	u_int32_t entryhi, entrylo, index;
	// entryhi = faultaddress;
	// entrylo = page | TLBLO_VALID | TLBLO_DIRTY;

	// trying to use probe first - if it exists, write to it
	// int tlb_index = TLB_Probe(entryhi, 0);
	// if (tlb_index >= 0){
	// 	TLB_Write(entryhi, entrylo, tlb_index);
	// }

	// else look for an invalid tlb entry
	for (index = 0; index < NUM_TLB; index++){
		TLB_Read(&entryhi, &entrylo, index);
		if (entrylo & TLBLO_VALID) continue;
			//fill first non-valid one
			entryhi = faultaddress;
			entrylo = page | TLBLO_VALID | TLBLO_DIRTY; 
			TLB_Write(entryhi, entrylo, index);
			splx(spl);
			return 0;
	}
	// no invalid ones => pick entry at random and expel it
	entryhi = faultaddress;
	entrylo = page | TLBLO_VALID;
	TLB_Random(entryhi, entrylo);

	splx(spl);
	return 0;
}

int
bad_fault(vaddr_t faultaddress, struct addrspace* as)
{
	// if ((faultaddress >= as->start_heap) && (faultaddress < as->start_heap + as->heap_size * PAGE_SIZE)){
	// 	return 0;
	// }
	if ((faultaddress >= as->start_heap) && (faultaddress < as->heap_top)){
		return 0;
	}
	if ((faultaddress >= as->code) && (faultaddress < as->code + as->code_size * PAGE_SIZE)){
		return 0;
	}
	if ((faultaddress >= as->data) && (faultaddress < as->data + as->data_size * PAGE_SIZE)){
		return 0;
	}
	if ((faultaddress < USERSTACK) && (faultaddress >= as->stack)){
		return 0;
	}
	return 1;
}

paddr_t
get_page(vaddr_t va, struct addrspace* as)
{
	// get indexes into directory
	vaddr_t table_index, page_index;

	// top 10 bits indexes the page directory
	table_index = va >> 22;
	// middle 10 bits indexes the particular page 
	page_index = (va & 0x003ff000) >> 12;

	assert(table_index < PT_SIZE);
	assert(page_index < PT_SIZE);

	// check if that directory exists
	if (as->as_ptes[table_index]){
		// check if the associated page exists
		if (as->as_ptes[table_index]->PTE[page_index]){
			// return this page table's address
			return (as->as_ptes[table_index]->PTE[page_index]);
		}
		// page doesn't exist but directory does, so make new page and save physical address of it
		paddr_t new_page;
		new_page = KVADDR_TO_PADDR(alloc_user_page(va));
		if (!new_page) return ENOMEM;
		// otherwise, set the page index to the address, and return the page
		as->as_ptes[table_index]->PTE[page_index] = new_page;
		return (as->as_ptes[table_index]->PTE[page_index]);
	}
	// if the directory doesn't exist, create it and the pages it should point to
	int i;
	as->as_ptes[table_index] = (struct as_pagetable*)kmalloc(sizeof(struct as_pagetable));
	// making sure that this is not NULL
	if (as->as_ptes[table_index] == NULL){
		return ENOMEM;
	}
	for (i = 0; i < PT_SIZE; i++){
		as->as_ptes[table_index]->PTE[i] = 0;
	}
	// now that the directory exists, create the page
	paddr_t new_page;
	new_page = KVADDR_TO_PADDR(alloc_user_page(va));
	if (new_page == ENOMEM){
		struct as_pagetable* temp;
		temp = as->as_ptes[table_index];
		as->as_ptes[table_index] = NULL;
		kfree(temp);
		return ENOMEM;
	}
	// otherwise, set the page index to the address, and return the page
	as->as_ptes[table_index]->PTE[page_index] = new_page;
	return (as->as_ptes[table_index]->PTE[page_index]);
}

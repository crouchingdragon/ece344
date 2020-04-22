
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
extern u_int32_t new_start;

extern size_t numofpgs;
struct addrspace *
as_create(void) //FIXME: Gets interrupted after the first line and ends up at alloc_pages (where I set the next breakpoint) when I hit n
{
    int spl = splhigh();

    kprintf("in as_create\n");
    struct addrspace *as = kmalloc(sizeof(struct addrspace));
    if (as==NULL) {
        return NULL;
    }

    // Initializations
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

    // First layer of PTE points to nothing yet
    int i;
    for (i = 0; i < PT_SIZE; i++){
        as->as_ptes[i] = NULL;
    }

    splx(spl);
    return as;
}
 
int
as_copy(struct addrspace *old, struct addrspace **ret)
{
//     kprintf("in as_copy\n");
//     int spl = splhigh();
//     struct addrspace *newas;
 
//     newas = as_create();
//     if (newas==NULL) {
//         panic("out of memory in as_copy");
//         return ENOMEM;
//     }

//     // save current regions
//     newas->start_heap = old->start_heap;
//     newas->heap_size = old->heap_size;
//     newas->stack = old->stack;
//     newas->stack_size = old->stack_size;
//     newas->code = old->code;
//     newas->code_size = old->code_size;
//     newas->data = old->data;
//     newas->data_size = old->data_size;
//     newas->perm = old->perm;
//     newas->old_perm = old->old_perm;
//     newas->adr_access = old->adr_access;


 
//     //COPYING REGIONS**************************************************************************
 
//     // int i;
//     // for (i = 0; i < array_getnum(old->as_regions); i++) {
//     //     struct as_region* copy = kmalloc(sizeof(struct as_region));
//     //     *copy = *((struct as_region*)array_getguy(old->as_regions, i));// not sure if these too lines fully copy stuff
//     //     array_add(newas->as_regions, copy);
//     // }
 
//     //COPYING HEAP*********************************************************************************
 
//     // newas->start_heap = old->start_heap;
//     // newas->end_heap = old->end_heap;
//     // newas->permissions = old->permissions;
// //is there anything im missing??? right now im copying the last 12 bits.. 1 (startbit) + 1(endbit) + 10(permission) = 12
 
//     //COPYING PAGE TABLE ENTRIES PTE*********************************************************************
 
//     int j;
//     for (j = 0; j < 1024; j++) {
//         if(old->as_ptes[j] != NULL) {
//             newas->as_ptes[j] = (struct as_pagetable*) kmalloc(sizeof(struct as_pagetable));
//             struct as_pagetable *src = old->as_ptes[j]; // was i before
//             struct as_pagetable *copying = newas->as_ptes[j]; //was i before
//             int k;
//             for (k = 0; k < 1024; k++) {
//                 copying->PTE[k] = 0;
//                 if(src->PTE[k] & 0x00000800) { 
//                     paddr_t src_paddr = (src->PTE[k] & PAGE_FRAME);
//                     vaddr_t dest_vaddr = (j << 22) + (k << 12);
//                     paddr_t dest_paddr = get_page(dest_vaddr, newas); // this function is at the bottom but i havent defined it yet as idk where i shd do it
//                     memmove((void *) PADDR_TO_KVADDR(dest_paddr),
//                     (const void*) PADDR_TO_KVADDR(src_paddr), PAGE_SIZE);
//                     copying->PTE[k] |= dest_paddr;
//                     copying->PTE[k] |= 0x00000800;
//                 }
//                 else{
//                     copying->PTE[k] = 0;
//                 }
//             }
//         }
//         else{
//             newas->as_ptes[j] = NULL;
//         }
//     }
               
//     //(void)old;
   
//     *ret = newas;
//     splx(spl);
    (void)old;
    (void)ret;
    return 0;
}
 
void
as_destroy(struct addrspace *as)
{
    kprintf("IN AS DESTROY\n");

    int spl = splhigh();
    // P(coremap_access);

    // Setting all regions to 0
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

    unsigned i, j, free_index;
    for (i = 0; i < PAGE_SIZE; i++){
        if (as->as_ptes[i] == NULL) continue;
        for (j = 0; j < PAGE_SIZE; j++){
            if (as->as_ptes[i]->PTE[j] == 0) continue;
            free_index = get_index(as->as_ptes[i]->PTE[j]);
            free_from_core(free_index);
            as->as_ptes[i]->PTE[j] = 0;
        }
        kfree(as->as_ptes[i]);
    }
    
    //V(coremap_access);
    kfree(as);
    splx(spl);
}

// from dumb vm
void
as_activate(struct addrspace *as)
{   
    // kprintf("in as_activate\n");   

	int i, spl;

	(void)as;

	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
 
    // (void)as;
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
    kprintf("in as_define_region\n");
    // int spl = splhigh();
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
    // kprintf("vaddr: %d   vaddr mod PAGE_FRAME: %d\n", vaddr, (vaddr % PAGE_FRAME));

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME; // extends upper end of the range

	npages = sz / PAGE_SIZE; // 

    // defining stack region
    as->stack = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;

    if((as->code == 0) && (as->code_size == 0)){
        as->code = vaddr;
        as->code_size = npages;
        as->perm = readable | writeable | executable;
        // as->start_heap = vaddr + sz;
        as->start_heap = vaddr + npages*PAGE_SIZE;
        // kprintf("code: %d   code mod page size = %d\n", as->code, (as->code % PAGE_FRAME));
        assert((as->code % PAGE_FRAME) == as->code);
        as->heap_size = 0;
        assert(as->start_heap != 0);
        // splx(spl);
        return 0;
    }
    if((as->data == 0) && (as->data_size == 0)){
        as->data = vaddr;
        as->data_size = npages;
        as->perm = readable | writeable | executable;
        // as->start_heap = vaddr + sz;
        as->start_heap = vaddr + npages*PAGE_SIZE;
        assert((as->data % PAGE_FRAME) == as->data);
        as->heap_size = 0; // number of pages in heap
        assert(as->start_heap != 0);
        // splx(spl);
        return 0;
    }
    panic("Didn't define regions");
    // splx(spl);
    return 0;
}
 
int
as_prepare_load(struct addrspace *as)
{
    kprintf("in as_prepare_load\n");   
    /*
     * Write this.
     */
    // as->old_perm = as->perm;
    // as->perm = PF_W | PF_R;
    
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
 
    (void)as;
    return 0;
}
 
int
as_complete_load(struct addrspace *as)
{
    kprintf("in as_complete_load\n");   
    /*
     * Write this.
     */
    // as->perm = as->old_perm;
    // as->old_perm = 0;

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

    (void)as;
    return 0;
}
 
int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
    kprintf("in as_define_stack\n");   

    // assert(as->as_stackpbase != 0);
    // (void)as;
    as->stack = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
    kprintf("In stack: stack top = %d\n", as->stack);
 
    /* Initial user-level stack pointer */
    *stackptr = USERSTACK;
   
    return 0;
}


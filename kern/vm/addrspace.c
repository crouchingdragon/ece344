#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>
#include <elf.h>

#include <array.h>
#include <synch.h> 
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
    as->as_regions = array_create();
    as->start_heap = 0;
    as->end_heap = 0;
   
    int i;
    for (i = 0; i < PT_SIZE; i++){
        as->as_ptes[i] = NULL;
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
        return ENOMEM;
    }
 
    //COPYING REGIONS**************************************************************************
 
    int i;
    for (i = 0; i < array_getnum(old->as_regions); i++) {
        struct as_region* copy = kmalloc(sizeof(struct as_region));
        *copy = *((struct as_region*)array_getguy(old->as_regions, i));// not sure if these too lines fully copy stuff
        array_add(newas->as_regions, copy);
    }
 
    //COPYING HEAP*********************************************************************************
 
    newas->start_heap = old->start_heap;
    newas->end_heap = old->end_heap;
    newas->permissions = old->permissions;
//is there anything im missing??? right now im copying the last 12 bits.. 1 (startbit) + 1(endbit) + 10(permission) = 12
 
    //COPYING PAGE TABLE ENTRIES PTE*********************************************************************
 
    int j;
    for (j = 0; j < 1024; j++) {
        if(old->as_ptes[j] != NULL) {
            newas->as_ptes[j] = (struct as_pagetable*) kmalloc(sizeof(struct as_pagetable));
            struct as_pagetable *src = old->as_ptes[i];
            struct as_pagetable *copying = newas->as_ptes[i];
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
    unsigned i;
    for (i = 0; i < numofpgs; i++) {
        if(Coremap[i].state != 0 && Coremap[i].addspace == as){
            Coremap[i].addspace = NULL;
            Coremap[i].vir_addspace = 0;
            Coremap[i].state = 0;
            Coremap[i].last = 0;
        }
    }
 
    array_destroy(as->as_regions);
   
    for(i = 0; i < 1024; i++) {
        if(as->as_ptes[i] != NULL)
            kfree(as->as_ptes[i]);
    }
   
    kfree(as);
    splx(spl);
}
 
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
	sz += vaddr & ~(vaddr_t)PAGE_FRAME; // lower 12 bits of vaddr == vaddr % PAGE_SIZE -- aligns vaddr to next lowest page addr without affecting upper end
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME; // extends upper end of the range

	npages = sz / PAGE_SIZE; // 

    // defining heap start and end
    as->start_heap = vaddr + sz;
    as->end_heap = as->start_heap;

    // allocate pages in the coremap to accomodate size of new region (don't need to be contiguous)
    P(coremap_access);
    int i;
	u_int32_t count, j;
	count = 0;
    for (j = 0; j < numofpgs; j++){
        if ((Coremap[j].state & FREE) && !(Coremap[j].state & DIRTY)){
            Coremap[j].vir_addspace = vaddr;
            Coremap[j].state = Coremap[j].state | DIRTY;
            count++;
        }
        if (count >= npages) break;
    }
    V(coremap_access);
    // Mark pages as allocated, update flags, update v_base, v_offset, permissions
    assert(as->as_regions != NULL);
	struct as_region *current;
    for(i = 0; i < array_getnum(as->as_regions); i++){
		current = array_getguy(as->as_regions, i);
        if (current->npgs == 0) break; // gets next empty region in array
    }
    current->bottom_vm = vaddr + sz - 1; // top vm is vaddr
    current->npgs = npages;
    current->region_permis = readable | writeable | executable;
    current->old_perm = -1; // no old

	array_setguy(as->as_regions, i, current);

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
    return EUNIMP;
}
 
int
as_prepare_load(struct addrspace *as)
{
    /*
     * Write this.
     */
    // change each regions page table permission to read-write since we're going to load content (code, data) into them
    // sets all permissions to write
    // you should not only overwrite the permission, but keep track of what it was before you overwrote it (as complete load restores this)
    int region_size = array_getnum(as->as_regions);
    int i;
	struct as_region *current;
    for (i = 0; i < region_size; i++){
		current = array_getguy(as->as_regions, i);
		current->old_perm = current->region_permis;
		current->region_permis = READ_WRITE; //FIXME: Not sure if this is the right read and write flag
		array_setguy(as->as_regions, i, current);
        // as->as_regions[i].old_perm = as->as_regions[i].region_permis;
        // as->as_regions[i].region_permis = READ | WRITE;
    }
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
    int region_size = array_getnum(as->as_regions);
    int i;
	struct as_region *current;
    for (i = 0; i < region_size; i++){
		current = array_getguy(as->as_regions, i);
		current->region_permis = current->old_perm;
		array_setguy(as->as_regions, i, current);
        // as->as_regions[i].region_permis = as->as_regions[i].old_perm;
    }
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
 
    assert(as->as_stackpbase != 0);
    // (void)as;
 
    /* Initial user-level stack pointer */
    *stackptr = USERSTACK;
   
    return 0;
}

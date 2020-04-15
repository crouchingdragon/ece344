#ifndef _ADDRSPACE_H_
#define _ADDRSPACE_H_
 
#include <vm.h>
#include "opt-dumbvm.h"
#include <machine/spl.h>
 
#define PT_SIZE 1024
 
struct vnode;
 
/*
 * Address space - data structure associated with the virtual memory
 * space of a process.
 *
 * You write this.
 */

// page size is 4KB
// an array of these is the second layer
// pte size should be 4 bytes = 32 bits = just a physical address I guess (whaaatttt???)
// struct pt_entry{
//     // how do you index? I remember CPU index and index in coremap somewhere
//     // u_int32_t frame_number:10; // right number of bits?
//     paddr_t paddr; // physical address of this page
//     off_t swap_addr; //swap address (in case its swapped to disk)
//     struct lock *page_lock; // don't modify this entry unless you have access
//     unsigned valid:1;
//     unsigned dirty:1;
//     u_int32_t permission;
// };

// // first 10 bits should index into this layer
// struct first_layer{
//     // struct pt_entry* PTE[PT_SIZE];
//     u_int32_t* PTE[PT_SIZE]; // pointer to the next page table (array of page table entries which simplpy hold physical addresses)
// }

// struct addr{
//     struct first_layer first[PT_SIZE]; // index into first layer depending on first 10 bits (which pointer to the next layer should be used)
//     vaddr_t heap_limit; // cannot grow upwards past this point
//     vaddr_t start_heap;
//     vaddr_t end_heap;
//     paddr_t stack_base;

// }

/*  First 10 bits of virtual address index into addrspace
    Next 10 bits index into whichever page table the particular
    addrspace points to
*/

struct as_pagetable{
    paddr_t PTE[PT_SIZE];
};
 
// struct as_region{
//     vaddr_t start;
    // vaddr_t end;
    // size_t npgs;
    // unsigned int region_permis;
    // unsigned int old_perm;
// };

// struct addrspace2{
//     vaddr_t vm_base;
//     paddr_t start_heap;
//     paddr_t end_heap;
//     paddr_t stackpbase;
//     u_int32_t permission;
//     u_int32_t old_permission;
//     u_int32_t npgs;
//     struct lock* adr_access;
//     struct as_pagetable* pages;
// };
 
struct addrspace {
#if OPT_DUMBVM
    vaddr_t as_vbase1;
    paddr_t as_pbase1;
    size_t as_npages1;
    vaddr_t as_vbase2;
    paddr_t as_pbase2;
    size_t as_npages2;
    paddr_t as_stackpbase;
#else
    // assuming all regions will always have the same permissions
    // struct as_region heap;
    // struct as_region stack;
    // struct as_region code;
    // struct as_region data;
    vaddr_t start_heap;
    vaddr_t end_heap;
    size_t heap_size;
    vaddr_t stack;
    size_t stack_size;
    vaddr_t code;
    size_t code_size;
    vaddr_t data;
    size_t data_size;
    unsigned int perm;
    unsigned int old_perm;
    struct lock* adr_access;
    struct as_pagetable *as_ptes[PT_SIZE];


    /* Put stuff here for your VM system */
    // u_int32_t permissions;
    // vaddr_t start_heap; // user heap start
    // vaddr_t end_heap; // user heap end
    // paddr_t as_stackpbase; // necessary?
    // struct vnode *vm_obj; // needed?
    // vaddr_t as_vbase1; 
    // struct as_pagetable *as_ptes[PT_SIZE];

    // vaddr_t vm_base;
    // paddr_t start_heap;
    // paddr_t end_heap;
    // paddr_t stackpbase;
    // u_int32_t permission;
    // u_int32_t old_permission;
    // u_int32_t npgs;
    // struct lock* adr_access;
    // struct as_pagetable* pages;

    ///////////// DUMB VM STUFF ////////////////
    // vaddr_t as_vbase1;
    // paddr_t as_pbase1;
    // size_t as_npages1;
    // vaddr_t as_vbase2;
    // paddr_t as_pbase2;
    // size_t as_npages2;
    // paddr_t as_stackpbase;

#endif
};
 
/*
 * Functions in addrspace.c:
 *
 *    as_create - create a new empty address space. You need to make
 *                sure this gets called in all the right places. You
 *                may find you want to change the argument list. May
 *                return NULL on out-of-memory error.
 *
 *    as_copy   - create a new address space that is an exact copy of
 *                an old one. Probably calls as_create to get a new
 *                empty address space and fill it in, but that's up to
 *                you.
 *
 *    as_activate - make the specified address space the one currently
 *                "seen" by the processor. Argument might be NULL,
 *        meaning "no particular address space".
 *
 *    as_destroy - dispose of an address space. You may need to change
 *                the way this works if implementing user-level threads.
 *
 *    as_define_region - set up a region of memory within the address
 *                space.
 *
 *    as_prepare_load - this is called before actually loading from an
 *                executable into the address space.
 *
 *    as_complete_load - this is called when loading from an executable
 *                is complete.
 *
 *    as_define_stack - set up the stack region in the address space.
 *                (Normally called *after* as_complete_load().) Hands
 *                back the initial stack pointer for the new process.
 */
 
struct addrspace *as_create(void);
int               as_copy(struct addrspace *src, struct addrspace **ret);
void              as_activate(struct addrspace *);
void              as_destroy(struct addrspace *);
 
int               as_define_region(struct addrspace *as,
                   vaddr_t vaddr, size_t sz,
                   int readable,
                   int writeable,
                   int executable);
int       as_prepare_load(struct addrspace *as);
int       as_complete_load(struct addrspace *as);
int               as_define_stack(struct addrspace *as, vaddr_t *initstackptr);
 
/*
 * Functions in loadelf.c
 *    load_elf - load an ELF user program executable into the current
 *               address space. Returns the entry point (initial PC)
 *               in the space pointed to by ENTRYPOINT.
 */
 
int load_elf(struct vnode *v, vaddr_t *entrypoint);
 
 
#endif /* _ADDRSPACE_H_ */

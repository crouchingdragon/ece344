#ifndef _VM_H_
#define _VM_H_
 
#include <machine/vm.h>
#include "opt-dumbvm.h"
#include <thread.h>
 
/*
 * VM system-related definitions.
 *
 * You'll probably want to add stuff here.
 */

#define DUMBVM_STACKPAGES    12
#define Firstlevel 0xffc00000 
#define Secondlevel 0x003ff000	
#define PAGE_FRAME 0xfffff000

struct Coremap_struct {
    // page stuff
    paddr_t phy_addspace;
    vaddr_t vir_addspace;
    off_t swap_addspace;
    // struct lock* page_lock;

    // core stuff
    int id:7;  
    struct addrspace* addspace;
    unsigned state:3; // freed = 0, fixed = 1, dirty = 2
    int last:2; // set this if its the last entry in a block
    // int length;
};

// struct page{
//     paddr_t phy_addr; // physical address of page
//     off_t swap_addr;
//     struct lock *page_lock;
// };

struct Coremap_struct *Coremap;
struct semaphore* coremap_access;

// struct page {
//     paddr_t paddr; // physical address of this page
//     off_t swap_addr; //swap address (in case its swapped to disk)
//     // add a spinlock, so no one can modify this page if it's being used
//     struct lock *page_lock;
// };
//Page size is 4k, so use lower pits of paddr for flags

// struct core_entry {
//     // u_int32_t status; // bit mask this to figure out if dirty, free, fixed clean
//     // struct core_entry *next; // should I make core_entry a linked list?
 
//     //Some stuff from vid
//     struct page *page;
//     unsigned cpu_index:4;
//     int tlb_index:7; //Have 64 entries in the TLB, index inside the TLB, if not in TLB, -1
//     int kernel:1;
//     int last_page:1;
//     int allocated:1;
//     int pinned:1;
// };

// #define KVADDR_TO_PADDR(vaddr) ((vaddr)-MIPS_KSEG0)

/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted*/

// read write permissions (necessary?)
#define READ    0
#define WRITE   1
#define READ_WRITE  2

// defining bit masks for flags
#define FREE 1 
#define FIXED 2
#define DIRTY 4

// diagnostic function used as a menu command
void
cmd_print_coremap(void);

// Helper function - Returns the virtual address of the next available page
int is_free(void);

// Checks that contiguous space of size npgs is available
u_int32_t has_space(int npgs);

/* Initialization function */
void vm_bootstrap(void);
 
/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress);
 
/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(int npages);

void free_kpages(vaddr_t addr);

int index_from_vaddr(vaddr_t);

vaddr_t alloc_one_page(void);

vaddr_t alloc_pages(int numofpgs);

vaddr_t alloc_oneormorepgs(int npgs);

u_int32_t* retEntry(struct thread* addrspace_owner, vaddr_t va);

paddr_t  load_seg(int id, struct addrspace* as, vaddr_t v_as);

int faults(vaddr_t faultaddress, unsigned int permissions);

void check_levels(vaddr_t faultaddress, paddr_t* paddr);

paddr_t alloc_page_userspace(struct addrspace * as, vaddr_t v_as);

void fault_stack(int faulttype, vaddr_t faultaddress, int* retval);

void fault_heap(int faulttype, vaddr_t faultaddress, int* retval, struct addrspace* as);

static
paddr_t
getppages(unsigned long npages);
#endif /* _VM_H_ */


// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run
{
  struct run *next;
};

struct
{
  struct spinlock lock;
  struct run *freelist;
} kmem;

// For CoW
// this counts the number of refs to a page that the cowpies have
// so, for a page with no cowpy, count is 0. Number of programs that can access it is 1
// ... for a page with 4 cowpies, count is 4. Number of programs that can access it is 5
struct
{
  struct spinlock lock;
  uint64 count;
} pg_cowrefs[PHYSTOP / PGSIZE];

void kinit()
{
  initlock(&kmem.lock, "kmem");
  // For CoW
  for (uint64 i = 0; i < PHYSTOP / PGSIZE; i++)
    initlock(&pg_cowrefs[i].lock, "pgcref");
  freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // For CoW
  // Calculate the index for the page in the pg_cowrefs array.
  int pageIndex = (uint64)pa / PGSIZE;

  // Acquire the lock for the page's reference count.
  acquire(&pg_cowrefs[pageIndex].lock);

  // Check for negative reference count, indicating a serious system error.
  if (pg_cowrefs[pageIndex].count < 0)
  {
    panic("kfree: cow page with negative reference count");
  }

  // If the page is shared (reference count > 0), decrement the count.
  if (pg_cowrefs[pageIndex].count > 0)
  {
    pg_cowrefs[pageIndex].count--;

    // Release the lock after modifying the count.
    release(&pg_cowrefs[pageIndex].lock);
    return;
  }

  // Release the lock if the page is not shared (reference count is 0).
  release(&pg_cowrefs[pageIndex].lock);

  // Till here for CoW
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
  {
    kmem.freelist = r->next;
    pg_cowrefs[(uint64)r / PGSIZE].count = 0; // Making sure that the page is not a cow page and thus making the count 0
  }
  release(&kmem.lock);

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}

int cowmappage(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
{
  // Validate the physical address (pa).
  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("cowmappage: invalid physical address");

  // Calculate the index for the page in the pg_cowrefs array.
  int pageIndex = pa / PGSIZE;

  // Acquire the lock for the page's reference count.
  acquire(&pg_cowrefs[pageIndex].lock);

  // Map the page and check the result.
  int mappingResult = mappages(pagetable, va, size, pa, perm | PTE_COW);

  // If mapping is successful, increment the page reference counter.
  if (mappingResult == 0)
    pg_cowrefs[pageIndex].count++;

  // Release the lock.
  release(&pg_cowrefs[pageIndex].lock);

  return mappingResult;
}

// duplicates a cow page to new physical location
// returns 0 on success
// returns -1 on not a valid page ( error should be handled in usertrap() )
// returns -2 if not enough memory for a new physical page
int dup_pg(pagetable_t pt, uint64 va)
{
  // Validate the virtual address (va).
  if (va >= MAXVA || va == 0)
    return -1;

  // Walk the page table to find the page table entry for va.
  pte_t *pte;
  if ((pte = walk(pt, va, 0)) == 0)
    return -1;

  // Check if the page is a COW page and accessible to the user.
  if ((*pte & PTE_COW) == 0)
    return 0; // not a cow'd addr

  // check if the page is accessible to the user
  if ((*pte & PTE_U) == 0)
    return -1; // not accessible to user

  // check if the page is valid
  if ((*pte & PTE_V) == 0)
    return -1; // not a valid page

  uint64 oldpa = PTE2PA(*pte);     // old physical address
  uint64 newpa = (uint64)kalloc(); // new physical address

  if (newpa == 0)
    return -2;
  memmove((void *)newpa, (void *)oldpa, PGSIZE);

  *pte = PA2PTE(newpa) | PTE_U | PTE_V | PTE_W | PTE_X | PTE_R;
  *pte &= ~PTE_COW;

  kfree((void *)oldpa);

  return 0;
}

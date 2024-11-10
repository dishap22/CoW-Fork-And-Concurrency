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

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock;
  int references[PHYSTOP >> PGSHIFT];
} page_references;

void inc_pg_ref(void *pa)
{
  acquire(&page_references.lock);
  if (page_references.references[(uint64)pa >> PGSHIFT] < 0) {
    panic("inc_pg_ref");
  }
  page_references.references[(uint64)pa >> PGSHIFT]++;
  release(&page_references.lock);
}

void dec_pg_ref(void *pa)
{
  acquire(&page_references.lock);
  if (page_references.references[(uint64)pa >> PGSHIFT] <= 0) {
    panic("dec_pg_ref");
  }
  page_references.references[(uint64)pa >> PGSHIFT]--;
  release(&page_references.lock);
}

void
kinit()
{
  initlock(&page_references.lock, "page_references");
  memset(page_references.references, 0, sizeof(page_references.references));
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  {
    acquire(&page_references.lock);
    page_references.references[(uint64)p >> PGSHIFT] = 1;
    release(&page_references.lock);
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // decrease page references
  // if we still have references, don't free the page
  dec_pg_ref(pa);
  acquire(&page_references.lock);
  if (page_references.references[(uint64)pa >> PGSHIFT] > 0) {
    release(&page_references.lock);
    return;
  }
  release(&page_references.lock);

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

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
  if(r) 
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
  {
    memset((char*)r, 5, PGSIZE); // fill with junk
    acquire(&page_references.lock);
    if (page_references.references[(uint64)r >> PGSHIFT] != 0) {
      panic("kalloc");
    }
    release(&page_references.lock);
    inc_pg_ref((void*)r);
  }
  return (void*)r;
}

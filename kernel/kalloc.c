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
  int count[(PGROUNDUP(PHYSTOP) - KERNBASE)/PGSIZE];
} PageRef;
void addRef(uint64 addr){
	acquire(&PageRef.lock);
	PageRef.count[addr2idx(addr)]++;
	release(&PageRef.lock);
}
void decRef(uint64 addr){
	acquire(&PageRef.lock);
	if(PageRef.count[addr2idx(addr)] == 0){
		release(&PageRef.lock);
		kfree((void*)addr);
	}
	else{
		PageRef.count[addr2idx(addr)]--;
		release(&PageRef.lock);
	}
}
int getRef(uint64 addr){
	acquire(&PageRef.lock);
	int idx = addr2idx(addr);
	int cnt;
	if(idx >= 0 && idx <= addr2idx(PHYSTOP)){
		cnt = PageRef.count[idx];
	}
	else{
		cnt = 0;
	}
	release(&PageRef.lock);
	return cnt;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
	initlock(&PageRef.lock, "PageRef");
  freerange(end, (void*)PHYSTOP);
	for(int i=0;i<=addr2idx(PHYSTOP);i++){
		PageRef.count[i] = 0;
	}
}

int
addr2idx(uint64 addr)
{
	int idx = (addr - (uint64)end) / PGSIZE;
	return idx;
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  if(getRef((uint64)pa) > 0){
		decRef((uint64)pa);
		return;
	}
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
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define HashSize 13
#define BUCKETSIZE 10
struct {
	struct spinlock Hashlock[HashSize];
  struct buf buf[HashSize][BUCKETSIZE];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[HashSize];
} bcache;

void
binit(void)
{
  struct buf *b;

  // initlock(&bcache.lock, "bcache");
	for(int i=0;i<HashSize;i++){
		char str[10];
		snprintf(str, sizeof(str), "bcache%d", i);
		initlock(&bcache.Hashlock[i], str);

		// Create linked list of buffers
		bcache.head[i].next = &bcache.head[i];
		snprintf(str, sizeof(str), "buffer%d", i);
		for(b = bcache.buf[i]; b < bcache.buf[i]+BUCKETSIZE; b++){
			b->next = bcache.head[i].next;
			initsleeplock(&b->lock, str);
			bcache.head[i].next = b;
		}
	}
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

	uint h = blockno % HashSize;
  acquire(&bcache.Hashlock[h]);

  // Is the block already cached?
  for(b = bcache.head[h].next; b != &bcache.head[h]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.Hashlock[h]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
	uint timeMin = 0;
	int flag = 0;
	struct buf *LRU = 0;
	for(b = bcache.head[h].next; b != &bcache.head[h]; b = b->next){
    if(b->refcnt == 0 && (flag == 0 || b->time < timeMin)){
			flag = 1;
			timeMin = b->time;
			LRU = b;
    }
  }
	if(flag != 0){
		LRU->dev = dev;
		LRU->blockno = blockno;
		LRU->valid = 0;
		LRU->refcnt = 1;
		release(&bcache.Hashlock[h]);
		acquiresleep(&LRU->lock);
		return LRU;
	}
	printf("flag = %d\n", flag);
	for(b = bcache.head[h].next; b != &bcache.head[h]; b = b->next){
    printf("b->dev = %d, b->blockno = %d, b->refcnt = %d, b->time = %d\n", b->dev, b->blockno, b->refcnt, b->time);
  }
	panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

	int h = b->blockno % HashSize;
  acquire(&bcache.Hashlock[h]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
		b->time = ticks;
  }
  
  release(&bcache.Hashlock[h]);
}

void
bpin(struct buf *b) {
	int h = b->blockno % HashSize;
  acquire(&bcache.Hashlock[h]);
  b->refcnt++;
  release(&bcache.Hashlock[h]);
}

void
bunpin(struct buf *b) {
	int h = b->blockno % HashSize;
  acquire(&bcache.Hashlock[h]);
  b->refcnt--;
  release(&bcache.Hashlock[h]);
}



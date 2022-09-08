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

#define NBUC 13  // the number of bucket of hashtable

extern uint ticks;

struct {
  struct spinlock lock;
  struct spinlock hashlock;
  struct buf buf[NBUF];
  int size;

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
} bcache;

struct bMem {
  struct spinlock lock;
  struct buf head;
};

static struct bMem hashTable[NBUC];

void
binit(void)
{
  struct buf *b;

  bcache.size = 0;

  initlock(&bcache.lock, "bcache");
  initlock(&bcache.hashlock, "bcache_hash");

  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  for(int i = 0; i < NBUC; i++){
    initlock(&(hashTable[i].lock), "bcache.bucket");
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    // bcache.head.next->prev = b;
    // bcache.head.next = b;
  }
}

// set value for LRU buffer
void 
replaceBuffer(struct buf *lruBuf, uint dev, uint blockno)
{
  lruBuf->dev = dev;
  lruBuf->blockno = blockno;
  lruBuf->valid = 0;
  lruBuf->refcnt = 1;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct buf  *tempPre, *tempBuf;
  struct buf *lruBuf = 0;

  // Is the block already cached?
  uint64 num = blockno % NBUC;
  acquire(&(hashTable[num].lock));
  for(b = hashTable[num].head.next; b; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&(hashTable[num].lock));
      acquiresleep(&b->lock);
      return b;
    }
  }

  acquire(&bcache.lock);
  if(bcache.size < NBUF){
    b = &bcache.buf[bcache.size++];
    release(&bcache.lock);
    replaceBuffer(b, dev, blockno);
    b->next = hashTable[num].head.next;
    hashTable[num].head.next = b;
    release(&(hashTable[num].lock));
    acquiresleep(&b->lock);
    return b;
  }
  release(&bcache.lock);
  release(&(hashTable[num].lock));

  acquire(&bcache.hashlock);
  for(int i = 0; i < NBUC; ++i){
    int minTick = -1;
    acquire(&(hashTable[num].lock));
    for(tempBuf = &(hashTable[num].head), b = tempBuf->next; b; tempBuf = b, b = b->next){
      if(num == blockno % NBUC && b->dev == dev && b->blockno == blockno){
        b->refcnt++;
        release(&(hashTable[num].lock));
        release(&bcache.hashlock);
        acquiresleep(&b->lock);
        return b;
      }
      if(b->refcnt == 0 && b->tick < minTick){
        lruBuf = b;
        tempPre = tempBuf;
        minTick = b->tick;
      }
    }

    if(lruBuf){
      replaceBuffer(lruBuf, dev, blockno);

      if(num != blockno % NBUC){
        tempPre->next = lruBuf->next;
        release(&(hashTable[num].lock));
        num = blockno % NBUC;
        acquire(&(hashTable[num].lock));
        lruBuf->next = hashTable[num].head.next;
        hashTable[num].head.next = lruBuf;
      }
      release(&(hashTable[num].lock));
      release(&bcache.hashlock);
      acquiresleep(&lruBuf->lock);
      return lruBuf;
    }
    release(&(hashTable[num].lock));
    if(++num == NBUC)
      num = 0;
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
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

  uint64 num = b->blockno % NBUC;
  acquire(&(hashTable[num].lock));
  b->refcnt--;
  if(b->refcnt == 0)
    b->tick = ticks;
  release(&(hashTable[num].lock));
  // b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
}

void
bpin(struct buf *b) {
  uint64 num = b->blockno % NBUC;
  acquire(&(hashTable[num].lock));
  b->refcnt++;
  release(&(hashTable[num].lock));
  // acquire(&bcache.lock);
  // b->refcnt++;
  // release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  uint64 num = b->blockno % NBUC;
  acquire(&(hashTable[num].lock));
  b->refcnt--;
  release(&(hashTable[num].lock));
  // acquire(&bcache.lock);
  // b->refcnt--;
  // release(&bcache.lock);
}



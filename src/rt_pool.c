#include "rtmp_proto.h"

#define POOL_GRANU 4096
#define POOL_EXTRA_SPACE  2048;

byte * rt_pool_alloc(POOL * ppool, unsigned int size)
{
  ppool->usedSize += size;
  if(size <= ppool->currCapa - ppool->currUsed){
    byte * p = ppool->curr->data + ppool->currUsed;
    ppool->currUsed += size;
    return p;
  } else {
    int newTrunkSize = size + POOL_EXTRA_SPACE;
    if(newTrunkSize < POOL_GRANU) {
      newTrunkSize = POOL_GRANU;
    }

    ppool->allocSpace += newTrunkSize;

    trunk_t * newTrunk = (trunk_t*)ALLOC(sizeof(trunk_t));

    newTrunk->data = ALLOC(newTrunkSize);
    newTrunk->prev = ppool->curr;

    ppool->currUsed = size;
    ppool->currCapa = newTrunkSize; 
    ppool->curr = newTrunk; 
    return ppool->curr->data;
  }
}

void rt_pool_init(POOL * ppool) {
  ppool->usedSize = 0;
  
  ppool->rootTrunk.prev = 0;
  ppool->rootTrunk.data = ALLOC(POOL_GRANU);

  ppool->curr = &(ppool->rootTrunk);
  ppool->currUsed = 0;
  ppool->currCapa = POOL_GRANU;

  ppool->listHead = NULL;
  ppool->listTail = NULL;
}

byte* rt_pool_allocz(POOL * ppool, int n, size_t sz)
{
  byte * buf = rt_pool_alloc(ppool, n * sz);
  memset(buf, 0, n * sz);
  return buf;
}


void rt_pool_reset(POOL * ppool)
{
  listnode * p = ppool->listHead;

  while(p) {
    //zmemlog << (void*)(p + 1) << endl;
    //  LOGD(p+1);
    p = p->next;
  }
  ppool->listHead = NULL;
  ppool->listTail = NULL;

  trunk_t * pTrunk = ppool->curr;

  while(pTrunk != &(ppool->rootTrunk)) {
    trunk_t * p = pTrunk->prev;
    /*LOGDA(pTrunk->data); delete [] pTrunk->data;
      LOGD(pTrunk); delete pTrunk;*/
    RELEASE(pTrunk->data);
    RELEASE(pTrunk);
    pTrunk = p;
  }
  ppool->currUsed = 0;
  ppool->currCapa = POOL_GRANU;
  ppool->curr = &(ppool->rootTrunk);
  ppool->allocSpace = POOL_GRANU;
  ppool->usedSize = 0;
}

void rt_pool_free(POOL * ppool)
{
  rt_pool_reset(ppool);
  //delete [] ppool->rootTrunk.data;
  RELEASE(ppool->rootTrunk.data);
}


/*
 * This file is part of Rushkit
 * Rushkit is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.

 * Rushkit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with Rushkit.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2009 Zeng Ke
 * Email: superisaac.ke@gmail.com
 */

#include "rushkit.h"

#define POOL_GRANU 2048
#define POOL_EXTRA_SPACE  2048;

byte * rt_pool_alloc(POOL * ppool, unsigned int size)
{
  ppool->used_size += size;
  if(size <= ppool->curr_capacity - ppool->curr_used){
    byte * p = ppool->curr->data + ppool->curr_used;
    ppool->curr_used += size;
    return p;
  } else {
    int newTrunkSize = size + POOL_EXTRA_SPACE;
    if(newTrunkSize < POOL_GRANU) {
      newTrunkSize = POOL_GRANU;
    }

    ppool->alloc_space += newTrunkSize;

    trunk_t * newTrunk = (trunk_t*)ALLOC(sizeof(trunk_t));

    newTrunk->data = ALLOC(newTrunkSize);
    newTrunk->prev = ppool->curr;

    ppool->curr_used = size;
    ppool->curr_capacity = newTrunkSize;
    ppool->curr = newTrunk;
    return ppool->curr->data;
  }
}

void rt_pool_init(POOL * ppool) {
  ppool->used_size = 0;

  ppool->root_trunk.prev = 0;
  ppool->root_trunk.data = ALLOC(POOL_GRANU);

  ppool->curr = &(ppool->root_trunk);
  ppool->curr_used = 0;
  ppool->curr_capacity = POOL_GRANU;

  ppool->list_head = NULL;
  ppool->list_tail = NULL;
}

byte* rt_pool_allocz(POOL * ppool, int n, size_t sz)
{
  byte * buf = rt_pool_alloc(ppool, n * sz);
  memset(buf, 0, n * sz);
  return buf;
}


void rt_pool_reset(POOL * ppool)
{
  ppool->list_head = NULL;
  ppool->list_tail = NULL;

  trunk_t * pTrunk = ppool->curr;

  while(pTrunk != &(ppool->root_trunk)) {
    trunk_t * p = pTrunk->prev;
    RELEASE(pTrunk->data);
    RELEASE(pTrunk);
    pTrunk = p;
  }
  ppool->curr_used = 0;
  ppool->curr_capacity = POOL_GRANU;
  ppool->curr = &(ppool->root_trunk);
  ppool->alloc_space = POOL_GRANU;
  ppool->used_size = 0;
}

void rt_pool_free(POOL * ppool)
{
  rt_pool_reset(ppool);
  //delete [] ppool->root_trunk.data;
  RELEASE(ppool->root_trunk.data);
}


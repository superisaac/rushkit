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

#include "rtmp_proto.h"

// Read Buffer
void rtmp_rb_init(READ_BUFFER * pbuffer, byte * data, int alen)
{
  pbuffer->buf = data;
  pbuffer->len = alen;
  pbuffer->current = data;
  pbuffer->need_more = 0;
}

byte * rtmp_rb_read_until(READ_BUFFER * pbuffer, int * cLen, byte c)
{
  byte * p;
  for(p = pbuffer->current; p < pbuffer->buf + pbuffer->len; p++){
    if(*p == c) {
      *cLen = p - pbuffer->current;
      byte * t = pbuffer->current;
      pbuffer->current = p + 1;
      return t;
    }
  }
  pbuffer->need_more = pbuffer->len;
  return NULL;
}


byte * rtmp_rb_read_e(READ_BUFFER * pbuffer, int nlen)
{
  int sz_now = rtmp_rb_get_space(pbuffer);
  assert(sz_now >= nlen);
  byte * t = pbuffer->current;
  pbuffer->current += nlen;
  return t;
}


double rtmp_rb_read_double_e(READ_BUFFER * pbuffer)
{
  double res = 0.0;
  byte * src = rtmp_rb_read_e(pbuffer, sizeof(double));
  load_data(src, sizeof(double), (byte*)(&res));
  return res;
}

short rtmp_rb_read_short_e(READ_BUFFER * pbuffer)
{
  short res = 0;
  byte * src = rtmp_rb_read_e(pbuffer, sizeof(short));
  load_data(src, sizeof(short), (byte*)(&res));
  return res;
}

byte rtmp_rb_read_byte_e(READ_BUFFER * pbuffer)
{
  byte byte = rtmp_rb_read_e(pbuffer, 1)[0];
  return byte;
}

int rtmp_rb_read_int_e(READ_BUFFER * pbuffer)
{
  int res = 0;
  byte * src = rtmp_rb_read_e(pbuffer, sizeof(int));
  load_data(src, sizeof(int), (byte*)(&res));
  return res;
}
int rtmp_rb_read_reverse_int_e(READ_BUFFER * pbuffer)
{
  int res = 0;
  byte * src = rtmp_rb_read_e(pbuffer, sizeof(int));
  load_data_reverse(src, sizeof(int), (byte*)(&res));
  return res;
}

int rtmp_rb_read_medium_int_e(READ_BUFFER * pbuffer)
{
  byte * src = rtmp_rb_read_e(pbuffer, 3);
  return (src[0] << 16) + (src[1] << 8) + src[2];
}


BOOLEAN rtmp_rb_assert_size(READ_BUFFER * pbuffer, int sz)
{
  if(sz  + pbuffer->current - pbuffer->buf > pbuffer->len ){
    pbuffer->need_more = sz + pbuffer->current - pbuffer->buf - pbuffer->len;
    return FALSE;
  }
  return TRUE;
}

int rtmp_rb_get_space(READ_BUFFER * pbuffer)
{
  int a=  pbuffer->buf + pbuffer->len - pbuffer->current;
  return a;
}

byte * rtmp_rb_read_string_e(READ_BUFFER * pbuffer, int * strlen)
{
  rtmp_rb_assert_size(pbuffer, 2);
  *strlen = rtmp_rb_read_short_e(pbuffer);
  pbuffer->current -= 2;
  return rtmp_rb_read_e(pbuffer, *strlen + 2) + 2;
}

int rtmp_rb_read(READ_BUFFER * pbuffer, byte ** dest, int nlen)
{
  int sz_now = rtmp_rb_get_space(pbuffer);
  *dest = pbuffer->current;

  if(sz_now >= nlen){
    pbuffer->current += nlen;
    return nlen;
  } else {
    pbuffer->current = pbuffer->buf + pbuffer->len;
    return sz_now;
  }
}

// Write buffer
void rtmp_wb_init(WRITE_BUFFER * pbuffer, POOL * pool, int capacity)
{
  pbuffer->pool = pool;
  pbuffer->capacity = capacity;
  pbuffer->buf = rt_pool_alloc(pbuffer->pool, capacity);
  pbuffer->current = pbuffer->buf;
}

int rtmp_wb_length(WRITE_BUFFER * pbuffer)
{
  return pbuffer->current - pbuffer->buf;
}

int rtmp_wb_write(WRITE_BUFFER * pbuffer, byte *data, int len)
{
  int sz_now = rtmp_wb_length(pbuffer);
  if(sz_now + len > pbuffer->capacity) {

    int new_cap = pbuffer->capacity * 2;

    while(sz_now + len > new_cap){
      new_cap *= 2;
    }

    byte * tmp_buff;
    tmp_buff = rt_pool_alloc(pbuffer->pool, new_cap);
    memcpy(tmp_buff, pbuffer->buf, sz_now);

    pbuffer->buf = tmp_buff;
    pbuffer->current = sz_now + pbuffer->buf;
    pbuffer->capacity = new_cap;
  }
  memcpy(pbuffer->current, data, len);
  pbuffer->current += len;
  return len;
}

int rtmp_wb_tell(WRITE_BUFFER * pbuffer)
{
  return pbuffer->current - pbuffer->buf;
}

byte * rtmp_wb_get_value(WRITE_BUFFER * pbuffer, int * len)
{
  *len = pbuffer->current - pbuffer->buf;
  return pbuffer->buf;
}


int rtmp_wb_write_short(WRITE_BUFFER * pbuffer, short src)
{
  byte dest[sizeof(short)];
  load_data((byte*)(&src), sizeof(short), dest);
  return rtmp_wb_write(pbuffer, dest, sizeof(short));
}

int rtmp_wb_write_reverse_int(WRITE_BUFFER * pbuffer, long src)
{
  int sz = 4;
  byte dest[sz]; //sizeof(src)];
  load_data_reverse((byte*)(&src), sz, dest);
  return rtmp_wb_write(pbuffer, dest, sz);
}

int rtmp_wb_write_int(WRITE_BUFFER * pbuffer, long src)
{
  int sz = 4;
  byte dest[sz];
  load_data((byte*)(&src), sz, dest);
  return rtmp_wb_write(pbuffer, dest, sz);
}


int rtmp_wb_write_byte(WRITE_BUFFER * pbuffer, byte src)
{
  return rtmp_wb_write(pbuffer, &src, 1);
}


int rtmp_wb_write_double(WRITE_BUFFER * pbuffer, double src)
{
  byte dest[sizeof(double)];
  load_data((byte*)(&src), sizeof(double), dest);
  return rtmp_wb_write(pbuffer, dest, sizeof(double));
}

int rtmp_wb_write_medium_int(WRITE_BUFFER * pbuffer, int e)
{
  byte d[3];
  d[0] = (e >> 16 ) & 0xff;
  d[1] = (e >> 8) & 0xff;
  d[2] = e & 0xff;
  return rtmp_wb_write(pbuffer, d, 3);
}

int rtmp_wb_write_string(WRITE_BUFFER * pbuffer, byte * str, int len)
{
  int short_len = rtmp_wb_write_short(pbuffer, len);
  return rtmp_wb_write(pbuffer, str, len) + short_len;
}

// fix sized Write buffer
void rtmp_fwb_reset(FIX_BUFFER * pfwb, POOL * ppool, int cap)
{
  pfwb->ppool = ppool;
  pfwb->capacity = cap;
  if(ppool) {
    pfwb->buf = rt_pool_alloc(ppool, cap);
  } else {
    pfwb->buf = ALLOC(cap);
  }
  pfwb->current = pfwb->buf;
}

void rtmp_fwb_init(FIX_BUFFER * pfwb)
{
  memset(pfwb, 0, sizeof(FIX_BUFFER));
}

void rtmp_fwb_free(FIX_BUFFER * pfwb)
{
  if(pfwb->ppool) {
    RELEASE(pfwb->buf);
  }
  rtmp_fwb_init(pfwb);
}

int rtmp_fwb_write(FIX_BUFFER * pfwb, byte * data, int len)
{
  int sz_now = rtmp_fwb_get_space(pfwb);
  assert(sz_now >= len);

  memcpy(pfwb->current, data, len);
  pfwb->current += len;
  return len;
}

int rtmp_fwb_get_space(FIX_BUFFER * pfwb)
{
  return pfwb->buf + pfwb->capacity - pfwb->current;
}

byte * rtmp_fwb_get_value(FIX_BUFFER * pfwb, int * len)
{
  *len = pfwb->current - pfwb->buf;
  return pfwb->buf;
}


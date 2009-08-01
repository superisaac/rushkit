/*
 * This file is part of Rushkit
 * Rushkit is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

int _pack_empty_string(byte * dest);
int _pack_end(byte * dest);

// Double
static double _unpack_double(AP * pap)
{
  double res = 0.0;
  pap->src += load_data(pap->src, sizeof(double), (byte*)(&res));
  return res;
}

static int _pack_double(double src, byte * dest)
{
  return load_data((byte*)(&src), sizeof(double), dest);
}

static tLONG  _unpack_long(AP * pap)
{
  tLONG res = 0;
  pap->src += load_data(pap->src, sizeof(tLONG), (byte*)(&res));
  return res;
}

static tLONG _pack_long(tLONG src, byte * dest)
{
  return load_data((byte*)(&src), sizeof(tLONG), dest);
}

// Short
static short  _unpack_short(AP * pap)
{
  short res = 0;
  pap->src += load_data(pap->src, sizeof(short), (byte*)(&res));
  return res;
}

static int _pack_short(short src, byte * dest)
{
  return load_data((byte*)(&src), sizeof(short), dest);
}


static int
_unpack_byte(AP * pap)
{
  return (*(pap->src++));
}

static int
_pack_byte(byte val, byte * dest)
{
  *dest = val;
  return 1;
}

int
_unpack_zstr(AP * pap, AK_STRING * pstr)
{
  pstr->size = _unpack_short(pap);
  pstr->content = rt_pool_alloc(pap->ppool, pstr->size);
  memcpy(pstr->content, pap->src, pstr->size);
  pap->src += pstr->size;
  return (pstr->size) + 2;
}

static int
_pack_string(AK_STRING * pstr, byte * dest)
{
  dest += _pack_short(pstr->size, dest);
  memcpy(dest, pstr->content, pstr->size);
  return pstr->size + 2;
}

void amf_print_string(AK_STRING * pstr)
{
  char * buffer = (char *)ALLOC(pstr->size + 5);
  buffer[0] = '\"';
  memcpy(buffer + 1, pstr->content, pstr->size);
  buffer[pstr->size + 1] = '\"';
  buffer[pstr->size + 2] = '\0';
  RELEASE(buffer);
  printf(buffer);
}

int _get_hash_pack_size(AK_TABLE * pvalue)
{
  int sum = 0;
  int i;
  AK_TABLE_ELEM * pelem = pvalue->elements;
  for(i=0;
      i< pvalue->size;
      pelem++, i++) {
    sum += pelem->key.size + sizeof(short);
    sum += amf_get_pack_size(&pelem->value);
  }
  sum += 3; // ""=> AMF_NIL
  return sum;
}


int _get_assoc_pack_size(AK_TABLE * pvalue)
{
  int sum = sizeof(tLONG);
  int i;
  AK_TABLE_ELEM * pelem = pvalue->elements;
  for(i=0;
      i< pvalue->size;
      pelem++, i++) {
    sum += pelem->key.size + sizeof(short);
    sum += amf_get_pack_size(&pelem->value);
  }
  return sum;
}


int _unpack_hash(AP * pap, AK_TABLE * ptable)
{
  WRITE_BUFFER buffer;
  rtmp_wb_init(&buffer, pap->ppool, 512);
  int rsum = 0;
  int count = 0;
  while(1) {
    AK_TABLE_ELEM elem;
    rsum += _unpack_zstr(pap, &(elem.key));
    rsum += amf_unpack(pap, &(elem.value));
    if(elem.value.type == AMF_END) {
      break;
    }
    rtmp_wb_write(&buffer, (byte*)(&elem), sizeof(elem));
    count ++;
  }
  int buf_sz = 0;
  ptable->elements = (AK_TABLE_ELEM*)rtmp_wb_get_value(&buffer, &buf_sz);
  ptable->size = count;
  return rsum;
}

int _pack_hash(AK_TABLE * ptable, byte * dest)
{
  int i;
  byte * p = dest;
  for(i = 0; i< ptable->size; i++) {
    AK_TABLE_ELEM * pelem = ptable->elements + i;
    p += _pack_string(&pelem->key, p);
    p += amf_pack(&(pelem->value), p);
  }
  p += _pack_empty_string(p);
  p += _pack_end(p);
  return p - dest;
}

int amf_print_hash(AK_TABLE * ptable)
{
  int i;
  printf("hash(");
  for(i = 0; i< ptable->size; i++) {
    AK_TABLE_ELEM * pelem = ptable->elements + i;
    amf_print_string(&pelem->key);
    printf("=>");
    amf_print(&pelem->value);
    if(i < ptable->size - 1) {
      printf(", ");
    }
  }
  printf(")");
}

int
_unpack_assoc_array(AP * pap, AK_TABLE * ptable)
{
  int size = _unpack_long(pap);
  int rsum = sizeof(tLONG);
  ptable->elements = (AK_TABLE_ELEM*)rt_pool_alloc(pap->ppool, size * sizeof(AK_TABLE_ELEM));
  int i=0;
  for(i=0; i<size; i++) {
    AK_TABLE_ELEM * pelem = ptable->elements + i;
    rsum += _unpack_zstr(pap, &(pelem->key));
    rsum += amf_unpack(pap, &(pelem->value));
    if(pelem->value.type == AMF_END) {
      break;
    }
  }
  ptable->size = i;
  return rsum;
}

int _pack_assoc_array(AK_TABLE * ptable, byte * dest)
{
  int i;
  byte * p = dest;
  p += _pack_long(ptable->size, p);
  for(i = 0; i< ptable->size; i++) {
    AK_TABLE_ELEM * pelem = ptable->elements + i;
    p += _pack_string(&pelem->key, p);
    p += amf_pack(&(pelem->value), p);
  }
  return p - dest;
}

int amf_print_assoc_array(AK_TABLE * ptable)
{
  int i;
  printf("assoc(");
  for(i = 0; i< ptable->size; i++) {
    AK_TABLE_ELEM * pelem = ptable->elements + i;
    amf_print_string(&pelem->key);
    printf("=>");
    amf_print(&pelem->value);
    if(i < ptable->size - 1) {
      printf(", ");
    }
  }
  printf(")");
}


int
_unpack_array(AP * pap, AK_ARRAY * parray)
{
  tLONG sz = _unpack_long(pap);
  int rsum = sizeof(tLONG);
  int i;
  parray->size = sz;
  parray->elements = (AV*)rt_pool_allocz(pap->ppool, sz, sizeof(AV));
  for(i = 0; i< sz; i++) {
    rsum += amf_unpack(pap, parray->elements + i);
  }
  return rsum;
}

int _get_array_pack_size(AK_ARRAY * parray)
{
  int sum = sizeof(tLONG);
  int i;
  AV * pav = parray->elements;
  for(i=0; i< parray->size; i++, pav++) {
    sum += amf_get_pack_size(pav);
  }
  return sum;
}

static int _pack_array(AK_ARRAY * parray, byte * dest)
{
  byte * p = dest;
  int i;
  p += _pack_long(parray->size, p);
  for(i=0; i< parray->size; i++) {
    AV * pav = parray->elements + i;
    p += amf_pack(pav, p);
  }
  return p - dest;
}

static int amf_print_array(AK_ARRAY * parray)
{
  int i;
  printf("[");
  for(i=0; i< parray->size; i++) {
    amf_print(parray->elements + i);
    if(i < parray->size - 1) {
      printf(", ");
    }
  }
  printf("]");
}


static AV * _alloc_value(AP * pap)
{
  AV * av = (AV *)rt_pool_allocz(pap->ppool, 1, sizeof(AV));
  return av;
}


int amf_unpack(AP * pap, AV * pvalue)
{
  if(pap->len + pap->start <= pap->src) {
    pvalue->type = AMF_END;
    return 0;
  }

  int type = _unpack_byte(pap);
  int rsum = sizeof(byte);
  pvalue->type = type;

  switch(type) {
  case AMF_NUMBER:
    pvalue->value.number_t = _unpack_double(pap);
    rsum += sizeof(double);
    break;
  case AMF_BOOLEAN:
    pvalue->value.bool_t = (BOOLEAN)_unpack_byte(pap);
    rsum += sizeof(byte);
    break;
  case AMF_STRING:
    rsum += _unpack_zstr(pap, &(pvalue->value.string_t));
    break;
  case AMF_ASSOC_ARRAY:
    rsum += _unpack_assoc_array(pap, &(pvalue->value.table_t));
    break;
  case AMF_HASH:
    rsum += _unpack_hash(pap, &(pvalue->value.table_t));
    break;
  case AMF_ARRAY:
    rsum += _unpack_array(pap, &(pvalue->value.array_t));
    break;
  case AMF_LINK:
    pvalue->value.link_t = _unpack_short(pap);
    rsum += sizeof(short);
    break;
  case AMF_NIL:
  case AMF_END:
  case AMF_UNDEF:
  case AMF_UNDEF_OBJ:
  default:
      break;
  }
  return rsum;
}

int amf_get_pack_size(AV * pvalue)
{
  int sz = 0;
  switch(pvalue->type) {
  case AMF_NUMBER:
    sz = sizeof(double);
    break;
  case AMF_BOOLEAN:
    sz = sizeof(byte);
    break;
  case AMF_STRING:
    sz = sizeof(short) + pvalue->value.string_t.size;
    break;
  case AMF_ASSOC_ARRAY:
    sz = _get_assoc_pack_size(&(pvalue->value.table_t));
    break;
  case AMF_HASH:
    sz = _get_hash_pack_size(&(pvalue->value.table_t));
    break;
  case AMF_ARRAY:
    sz = _get_array_pack_size(&(pvalue->value.array_t));
    break;
  case AMF_LINK:
    sz = sizeof(short);
    break;
  case AMF_NIL:
  case AMF_END:
  case AMF_UNDEF:
  case AMF_UNDEF_OBJ:
  default:
    sz = 0;
    break;
  }
  return sz + 1;
}

int _pack_end(byte * dest)
{
  AV v;
  v.type = AMF_END;
  return amf_pack(&v, dest);
}

int _pack_empty_string(byte * dest)
{
  AK_STRING str;
  str.size = 0;
  str.content = (byte*)"";
  return _pack_string(&str, dest);
}

void amf_print(AV * pvalue)
{
  switch(pvalue->type) {
  case AMF_NUMBER:
    printf("%f", pvalue->value.number_t);
    break;
  case AMF_BOOLEAN:
    printf(pvalue->value.bool_t? "true": "false");
    break;
  case AMF_STRING:
    amf_print_string(&pvalue->value.string_t);
    break;
  case AMF_ASSOC_ARRAY:
    amf_print_assoc_array(&(pvalue->value.table_t));
    break;
  case AMF_HASH:
    amf_print_hash(&(pvalue->value.table_t));
    break;
  case AMF_ARRAY:
    amf_print_array(&(pvalue->value.array_t));
    break;
  case AMF_LINK:
    printf("link(%d)", pvalue->value.link_t);
    break;
  case AMF_NIL:
  case AMF_END:
  case AMF_UNDEF:
  case AMF_UNDEF_OBJ:
  default:
    printf("amf(%d)", pvalue->type);
    break;
  }
}

byte * amf_pack_arguments(POOL * ppool, int count, AV * arguments, int * data_len)
{
  int i;
  int sum_cap = 0;
  for(i = 0; i< count; i++) {
    sum_cap += amf_get_pack_size(arguments + i);
  }

  byte * buffer = rt_pool_alloc(ppool, sum_cap);
  byte * orig = buffer;
  for(i= 0; i< count; i++) {
    buffer += amf_pack(arguments + i, buffer);
  }
  assert(sum_cap == buffer - orig);
  *data_len = sum_cap;
  return orig;
}

byte * amf_pack_call(POOL * ppool, char * method_name,
		     double req_id, int count, AV * arguments, int * data_len)
{
  int i;
  int sum_cap = 0;
  AV headers[3];
  amf_new_string(ppool, &headers[0], method_name);
  amf_new_number(headers + 1, req_id);
  headers[2].type = AMF_NIL;

  for(i=0; i< 3; i++) {
    sum_cap += amf_get_pack_size(headers + i);
  }

  for(i = 0; i< count; i++) {
    sum_cap += amf_get_pack_size(arguments + i);
  }


  byte * buffer = rt_pool_alloc(ppool, sum_cap);
  byte * orig = buffer;
  for(i= 0; i< 3; i++) {
    buffer += amf_pack(headers + i, buffer);
  }
  for(i= 0; i< count; i++) {
    buffer += amf_pack(arguments + i, buffer);
  }
  assert(sum_cap == buffer - orig);
  *data_len = sum_cap;
  return orig;
}


int amf_pack(AV * pvalue, byte * dest)
{
  int sz = 0;
  byte * p = dest;
  // Pack type
  p += _pack_byte(pvalue->type, p);

  switch(pvalue->type) {
  case AMF_NUMBER:
    p += _pack_double(pvalue->value.number_t, p);
    break;
  case AMF_BOOLEAN:
    p += _pack_byte(pvalue->value.bool_t, p);
    break;
  case AMF_STRING:
    p+= _pack_string(&pvalue->value.string_t, p);
    break;
  case AMF_ASSOC_ARRAY:
    p += _pack_assoc_array(&(pvalue->value.table_t), p);
    break;
  case AMF_HASH:
    p += _pack_hash(&(pvalue->value.table_t), p);
    break;
  case AMF_ARRAY:
    p += _pack_array(&(pvalue->value.array_t), p);
    break;
  case AMF_LINK:
    p += _pack_short(pvalue->value.link_t, p);
    break;
  case AMF_NIL:
  case AMF_END:
  case AMF_UNDEF:
  case AMF_UNDEF_OBJ:
  default:
    break;
  }
  return p - dest;
}


AP * amf_ap_alloc(POOL * ppool, byte * data, int dataLen)
{
  AP * pap = (AP *)rt_pool_alloc(ppool, sizeof(AP));
  pap->ppool = ppool;
  pap->len = dataLen;
  pap->start = data;
  pap->src = pap->start;
  return pap;
}

AV * amf_new_string(POOL * ppool, AV * dest, char * data)
{
  int count = strlen(data);
  dest->type = AMF_STRING;
  amf_new_count_string(ppool, &(dest->value.string_t), count, data);
  //dest->value.string_t.size = count;
  //dest->value.string_t.content = (byte*)data;
  return dest;
}

AK_STRING * amf_new_count_string(POOL * ppool, AK_STRING * dest, int count, char * data)
{
  if(ppool != NULL) {
    byte * tmpdata = rt_pool_alloc(ppool, count);
    memcpy(tmpdata, data, count);
    data = (char*)tmpdata;
  }
  dest->size = count;
  dest->content = (byte*)data;
  return dest;
}

AV * amf_new_number(AV * dest, double value)
{
  dest->type = AMF_NUMBER;
  dest->value.number_t = value;
  return dest;
}

AV * amf_new_bool(AV * dest, byte value)
{
  dest->type = AMF_BOOLEAN;
  dest->value.bool_t = value;
  return dest;
}

int amf_string_equal(AV * str_v, char * other)
{
  assert(str_v->type == AMF_STRING);
  int data_size = str_v->value.string_t.size;
  if(data_size != strlen(other)) {
    return 0;
  }
  return memcmp(str_v->value.string_t.content, other, data_size) == 0;
}

AV * amf_new_array(POOL * ppool, AV * dest, int count, AV * elements)
{
  if(ppool != NULL) {
    AV * tmpelems = (AV*)rt_pool_alloc(ppool, count * sizeof(AV));
    memcpy(tmpelems, elements, count * sizeof(AV));
    elements = tmpelems;
  }

  dest->type = AMF_ARRAY;
  dest->value.array_t.size = count;
  dest->value.array_t.elements = elements;
  return dest;
}

AK_TABLE_ELEM *
amf_new_table_elem_list(POOL * ppool, AK_TABLE_ELEM * elements, int count, char ** keys, AV * values)
{
  int i;
  for(i=0; i< count; i++) {
    //amf_new_string(ppool, &(elements[i].key), keys[i]);
    elements[i].key.size = strlen(keys[i]);
    elements[i].key.content = (byte*)keys[i];
    elements[i].value = values[i];
  }
  return elements;
}


AV * _amf_new_table(POOL * ppool, AV * dest, int table_type, int count, AK_TABLE_ELEM * elements)
{
  if(ppool != NULL) {
    AK_TABLE_ELEM * tmpelems = (AK_TABLE_ELEM*)rt_pool_alloc(ppool, count * sizeof(AK_TABLE_ELEM));
    memcpy(tmpelems, elements, count * sizeof(AK_TABLE_ELEM));
    elements = tmpelems;
  }
  dest->type = table_type;
  dest->value.table_t.size = count;
  dest->value.table_t.elements = elements;
  return dest;
}

AV *
amf_new_assoc_array(POOL * ppool, AV * dest, int count, AK_TABLE_ELEM * elements)
{
  return _amf_new_table(ppool, dest, AMF_ASSOC_ARRAY, count, elements);
}

AV *
amf_new_hash(POOL * ppool, AV * dest, int count, AK_TABLE_ELEM * elements)
{
  return _amf_new_table(ppool, dest, AMF_HASH, count, elements);
}

AV * amf_ap_read_next(AP * pap) {
  AV * pav = _alloc_value(pap);
  amf_unpack(pap, pav);
  return pav;
}

int amf_ap_is_end(AP * pap)
{
  return pap->src >= pap->start + pap->len;
}

AV * amf_ap_read_arguments(AP * pap, AK_ARRAY * parray)
{
  WRITE_BUFFER buffer;
  rtmp_wb_init(&buffer, pap->ppool, 512);
  int count = 0;
  while(!amf_ap_is_end(pap)) {
    AV av;
    amf_unpack(pap, &av);
    count ++;
    rtmp_wb_write(&buffer, (byte*)&av, sizeof(AV));
  }
  int buf_sz =0;
  parray->elements =  (AV*)rtmp_wb_get_value(&buffer, &buf_sz);
  parray->size = count;
  return parray->elements;
}

AV * amf_ap_read_array(AP * pap, int *len)
{
  WRITE_BUFFER buffer;
  rtmp_wb_init(&buffer, pap->ppool, 256);
  int count = 0;
  while(1) {
    AV av;
    amf_unpack(pap, &av);
    if(av.type == AMF_END) {
      break;
    }
    count ++;
    rtmp_wb_write(&buffer, (byte*)&av, sizeof(AV));
  }
  *len = count;
  int buf_sz =0;
  return (AV*)rtmp_wb_get_value(&buffer, &buf_sz);
}


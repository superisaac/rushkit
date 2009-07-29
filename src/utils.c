#include "rtmp_proto.h"

int system_is_small_endian = 1;


static inline int __load_data(byte * src, int len, byte * dest, int is_small_endian)
{
  if(is_small_endian) {
    dest += len - 1;
    register int i;
    for(i=0; i< len; i++){
      *dest = *src;      
      src ++;
      dest --;
    }
  } else {
    memcpy(dest, src, len);
  }
  return len;
}

int probe_small_endian()
{
  short n = 1942;
  byte buf[2];
  memcpy(buf, (char*)&n, 2);
  system_is_small_endian = (buf[0] != 0x07);
}


int load_data(byte * src, int len, byte * dest)
{
  return __load_data(src, len, dest, system_is_small_endian);
}

int load_data_reverse(byte * src, int len, byte * dest)
{
  return __load_data(src, len, dest, !system_is_small_endian);
}


int pack_short(byte * buf, short value)
{
  return load_data((byte*)&value, sizeof(value), buf);
}

int pack_long(byte * buf, tLONG value)
{
  return load_data((byte*)&value, sizeof(value), buf);
}

int unpack_int(byte * buf)
{
  int value;
  load_data(buf, sizeof(value), (byte*)(&value));
  return value;
}


short unpack_short(byte * buf)
{
  short value;
  load_data(buf, sizeof(value), (byte*)(&value));
  return value;
}

void 
trace_buffer(const byte * buffer, int len, char * prompt, int limit)
{
  int i;
  printf("%s(%d) ", prompt, len);
  for(i=0; i< len; i++) {
    if(isprint(buffer[i])) {
      printf("%c", buffer[i]);
    } else {
      printf("\\x%02x", (unsigned char)buffer[i]);
    }
    if(i>limit) {
      printf("...");
      break;
    }
  }
  printf("\n");
}


const char * 
trace_str(const byte * buffer, int len, char * prompt, int limit)
{
  static char outbuffer[256];
  char * p = outbuffer;
  int i;
  sprintf(p, "%s(%d) ", prompt, len);
  p += strlen(p);
  for(i=0; i< len; i++) {
    if(isprint(buffer[i])) {
      *p++ = buffer[i];
    } else {
      sprintf(p, "\\x%02x", (unsigned char)buffer[i]);
      p += strlen(p);
      
    }
    if(i>limit) {
      sprintf(p, "...");
      p += 3;
      break;
    }
  }
  *p = '\0';
  return outbuffer;
}

byte * ALLOC(size_t size)
{
  return (byte*)malloc(size);
}

byte * ALLOCZ(size_t size)
{
  byte * ptr = ALLOC(size);
  memset(ptr, 0, size);
  return ptr; 
}

void RELEASE(void * ptr)
{
  free(ptr);
}

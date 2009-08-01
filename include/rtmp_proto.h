#ifndef __RTMP_PROTO_H__
#define __RTMP_PROTO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#define LOGA(d)
#define LOGAA(d)
#define LOGD(d)
#define LOGDA(d)

#define  PAC_CHUNK_SIZE 0x01
#define  PAC_BYTES_READ 0x03
#define  PAC_PING 0x4
#define  PAC_INVALID 0x89

#define  PAC_INVOKE 0x14
#define  PAC_NOTIFY 0x12
    
#define  PAC_AUDIO  0x8
#define  PAC_VIDEO  0x9

#define CHANNEL_NEW 0
#define CHANNEL_SAME_SOURCE 1
#define CHANNEL_TIMER_CHANGE 2
#define CHANNEL_CONTINUE 3

#define AMF_NUMBER   0
#define AMF_BOOLEAN   1
#define AMF_STRING   2
#define AMF_HASH   3
#define AMF_NIL   5
#define AMF_UNDEF   6
#define AMF_LINK   7
#define AMF_ASSOC_ARRAY   8
#define AMF_END   9
#define AMF_ARRAY   10
#define AMF_DATE   11
#define AMF_LONG_STRING   12
#define AMF_UNDEF_OBJ   13
#define AMF_XML   15
#define AMF_CUSTOM   16

  typedef unsigned char byte;
  typedef int32_t tLONG;
  typedef int BOOLEAN;

#define FALSE 0
#define TRUE 1

#define STREQ(a, b) (strcmp(a, b) == 0)

  byte * ALLOC(size_t size);
  byte * ALLOCZ(size_t size);
  void RELEASE(void * ptr);

  typedef struct _trunk_t
  {
    struct _trunk_t * prev;
    byte * data;
  }trunk_t;

  typedef struct _listnode
  {
    struct _listnode * next;
  } listnode;

  typedef struct _pool_t
  {
    listnode * list_head;
    listnode * list_tail;

    trunk_t  root_trunk;
    trunk_t * curr;
    unsigned int curr_used;
    unsigned int curr_capacity;

    int used_size; 
    int alloc_space;

  } POOL;

  void rt_pool_init(POOL * ppool);
  byte * rt_pool_alloc(POOL * ppool, unsigned int size);
  byte* rt_pool_allocz(POOL * ppool, int n, size_t sz);
  void rt_pool_reset(POOL * ppool);
  void rt_pool_free(POOL * ppool);

  int load_data(byte * src, int len, byte * dest);
  int load_data_reverse(byte * src, int len, byte * dest);
  int pack_short(byte * buf, short value);
  int pack_long(byte * buf, tLONG value);
  int unpack_int(byte * buf);
  short unpack_short(byte * buf);
  void trace_buffer(const byte * buffer, int len, char* prompt, int limit);
  const char * trace_str(const byte * buffer, int len, char* prompt, int limit);

#define BUFFER_SZ 8192

  typedef struct read_buffer_t
  {
    byte * buf;
    byte * current;
    int len;
    int need_more;
  }READ_BUFFER;

  void rtmp_rb_init(READ_BUFFER * pbuffer, byte * data, int alen);
  byte * rtmp_rb_read_until(READ_BUFFER * pbuffer, int * cLen, byte c);
  byte * rtmp_rb_read_e(READ_BUFFER * pbuffer, int nlen);
  double rtmp_rb_read_double_e(READ_BUFFER * pbuffer);
  short rtmp_rb_read_short_e(READ_BUFFER*);
  byte rtmp_rb_read_byte_e(READ_BUFFER * pbuffer);
  int rtmp_rb_read_int_e(READ_BUFFER * pbuffer);
  int rtmp_rb_read_reverse_int_e(READ_BUFFER * pbuffer);
  int rtmp_rb_read_medium_int_e(READ_BUFFER * pbuffer);
  BOOLEAN rtmp_rb_assert_size(READ_BUFFER * pbuffer, int sz);
  int rtmp_rb_get_space(READ_BUFFER * pbuffer);
  byte *rtmp_rb_read_string_e(READ_BUFFER * pbuffer, int * strlen);
  int rtmp_rb_read(READ_BUFFER * pbuffer, byte ** dest, int nlen);


  typedef struct write_buffer_t
  {
    byte * buf;
    int capacity;
    byte * current;
    POOL * pool;
  } WRITE_BUFFER;

  void rtmp_wb_init(WRITE_BUFFER * pbuffer, POOL * pool, int capacity);
  int rtmp_wb_write(WRITE_BUFFER * pbuffer, byte *data, int len);
  int rtmp_wb_tell(WRITE_BUFFER * pbuffer);
  int rtmp_wb_length(WRITE_BUFFER * pbuffer);
  byte * rtmp_wb_get_value(WRITE_BUFFER * pbuffer, int * len);
  int rtmp_wb_write_short(WRITE_BUFFER * pb, short src);
  int rtmp_wb_write_reverse_int(WRITE_BUFFER * pbuffer, long src);
  int rtmp_wb_write_int(WRITE_BUFFER * pbuffer, long src);
  int rtmp_wb_write_byte(WRITE_BUFFER * pbuffer, byte src);
  int rtmp_wb_write_double(WRITE_BUFFER * pbuffer, double src);
  int rtmp_wb_write_medium_int(WRITE_BUFFER * pbuffer, int e);
  int rtmp_wb_write_string(WRITE_BUFFER * pbuffer, byte * str, int len);
  int probe_small_endian();

  typedef struct fix_buffer_t 
  {
    byte * buf;
    byte * current;
    int capacity;
    POOL * ppool;
  } FIX_BUFFER;

  void rtmp_fwb_reset(FIX_BUFFER * pfwb, POOL * ppool, int cap);
  void rtmp_fwb_init(FIX_BUFFER * pfwb);
  void rtmp_fwb_free(FIX_BUFFER * pfwb);
  int rtmp_fwb_write(FIX_BUFFER * pfwb, byte * data, int len);
  int rtmp_fwb_get_space(FIX_BUFFER * pfwb);
  byte * rtmp_fwb_get_value(FIX_BUFFER * pfwb, int * len);

  typedef struct _fame_t
  {
    int used;
    int timer;
    int size;
    int data_type; 
    int stream_id;
  } CHANNEL;

  typedef struct
  {
    int channel_id;
    CHANNEL channel;
    byte * data;
    int dataLen;
  } PACKET;

  struct rtmp_proto_t;
  struct method_table_t;

  typedef struct rtmp_proto_t {
    FIX_BUFFER tmp_buf;
    POOL pool;
    int ack_back_len;
    int pkg_st;
    void * user_data;
    struct method_table_t * method_table;
    size_t last_data_length;
    byte * last_data; 
    byte * readBuffer;
    int read_chunk_size;
    int write_chunk_size;

    int state;
    int packet_mask;
    int channel_id;

    CHANNEL * channels_in;
    CHANNEL * channels_out;

    FIX_BUFFER data_buffer;
    int last_timers[64];
  
    int num_up_bytes;
    int num_down_bytes;
    clock_t last_tm_bytes_read;

    double next_request_id;
  } RTMP_PROTO;

  typedef RTMP_PROTO * PPROTO;

  struct amf_value_t;

  typedef struct method_table_t {
    /* callbacks */
    int (*on_write_data)(PPROTO proto, byte * data, int len);

    void (*on_packet)(PPROTO proto, PACKET *);
    void (*on_packet_chunk_size)(PPROTO proto, PACKET *);
    void (*on_packet_invalid)(PPROTO proto, PACKET *);
    void (*on_packet_bytes_read)(PPROTO proto, PACKET *);
    void (*on_packet_ping)(PPROTO proto, PACKET *);
    void (*on_packet_invoke)(PPROTO proto, PACKET *);
    void (*on_packet_audio)(PPROTO proto, PACKET *);
    void (*on_packet_video)(PPROTO proto, PACKET *);
    void (*on_amf_call)(PPROTO prot, int channle_id, 
			double request_id, struct amf_value_t * method_name, 
			int argc, struct amf_value_t * args);
    void (*on_log)(PPROTO proto, char * level, char * message);

  } RTMP_METHOD_TABLE;

  void rtmp_proto_method_table_init(RTMP_METHOD_TABLE *);
  byte * rtmp_proto_prepare_buffer(PPROTO proto, size_t * plen);
  int rtmp_proto_send_packet(PPROTO proto, POOL * pool, PACKET * pac, BOOLEAN timer_relative);
  void rtmp_proto_feed(PPROTO proto, int len);
  void rtmp_proto_feed_data(PPROTO proto, const byte * data, int len);
  void rtmp_proto_init(PPROTO proto, RTMP_METHOD_TABLE *);
  void rtmp_proto_free(PPROTO proto);
  CHANNEL * rtmp_proto_get_channel(PPROTO proto);
  void rtmp_proto_call(PPROTO proto, char * method_name, int argc, struct amf_value_t * argv);
  void rtmp_proto_send_bytes_read(PPROTO proto);

  void * rtmp_proto_get_user_data(PPROTO proto);
  void rtmp_proto_set_user_data(PPROTO proto, void * user_data);
  void rtmp_proto_log(PPROTO proto, char * level, char * message);

  PACKET * rtmp_packet_get_packet(PPROTO proto, READ_BUFFER * Buffer);
  void rtmp_packet_reset(PPROTO proto);
  void rtmp_packet_init(PPROTO proto);
  void rtmp_packet_clear_out_channel(PPROTO proto, int channel_id);
  void rtmp_packet_clear_in_channel(PPROTO proto, int channel_id);
  void rtmp_packet_delete_out_channel(PPROTO proto, int channel_id);
  void rtmp_packet_delete_in_channel(PPROTO proto, int channel_id);
  int rtmp_packet_write_packet(PPROTO proto, WRITE_BUFFER * buffer, PACKET * pac, BOOLEAN timer_relative);
  int rtmp_packet_get_last_timer(PPROTO proto, int fn);


  // AMF pickler
  typedef struct amf_pickler_t {
    int len;
    POOL * ppool;
    byte * start;
    byte * src;
  } AP;

  typedef struct amf_array_t {
    int size;
    struct amf_value_t * elements;
  } AK_ARRAY;

  typedef struct amf_string_t {
    int size;
    byte * content;
  } AK_STRING;

  struct amf_table_elem_t;

  typedef struct amf_assoc_t {
    int size;
    struct amf_table_elem_t * elements;
  } AK_TABLE;

  typedef struct amf_value_t{
    int type;
    union {
      byte bool_t;
      short link_t;
      double number_t;
      AK_ARRAY array_t;
      AK_STRING string_t;
      AK_TABLE table_t;    
    } value;
  } AV;

  typedef struct amf_table_elem_t {
    AK_STRING key;
    AV value;
  } AK_TABLE_ELEM;


  AP * amf_ap_alloc(POOL * ppool, byte * data, int dataLen);
  AV * amf_ap_read_next(AP * pap);
  AV * amf_ap_read_array(AP * pap, int *len);
  AV * amf_ap_read_arguments(AP * pap, AK_ARRAY * parray);
  AV * amf_ap_read_next(AP * pap);
  byte * amf_pack_call(POOL * ppool, char * method_name, 
		       double req_id, int count, AV * arguments, int * data_len);

  int amf_get_pack_size(AV * pvalue);
  AV * amf_new_string(POOL * ppool, AV * dest, char * data);
  AK_STRING * amf_new_count_string(POOL * ppool, AK_STRING * dest, int count, char * data);
  AV * amf_new_number(AV * dest, double value);
  AV * amf_new_bool(AV * dest, byte balue);
  AV * amf_new_array(POOL * ppool, AV * dest, int count, AV * elements);
  AV * amf_new_assoc_array(POOL * ppool, AV * dest, int count, AK_TABLE_ELEM * elements);
  AV * amf_new_hash(POOL * ppool, AV * dest, int count, AK_TABLE_ELEM * elements);
  AK_TABLE_ELEM * amf_new_table_elem_list(POOL * ppool, AK_TABLE_ELEM * elements, int count, char ** keys, AV * values);
  int amf_string_equal(AV * str_v, char * other);

  int amf_unpack(AP * pap, AV * pvalue);
  int amf_pack(AV * pvalue, byte * dest);
  byte * amf_pack_arguments(POOL *, int count, AV * arguments, int * data_len);
  void amf_print(AV *);

  void amf_print_string(AK_STRING * pstr);

  int rtmp_proto_packet_return(PPROTO proto, POOL * ppool, int channel_id, double request_id, AV * ret_val);

#ifdef __cplusplus 
}
#endif

#endif

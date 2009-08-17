%module pyrushkit

%{
#include <Python.h>
#include "rushkit.h"
%}
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

%apply PPROTO INPUT {PPROTO proto};
  void rtmp_proto_method_table_init(RTMP_METHOD_TABLE *);
  void rtmp_proto_init(PPROTO proto, RTMP_METHOD_TABLE *);
//void * rtmp_proto_get_user_data(PPROTO proto);
//void rtmp_proto_set_user_data(PPROTO proto, void * user_data);

#if defined(SWIGPYTHON)
void environment_init();
void init_responder(PPROTO  proto, PyObject * responder);
void free_responder(PPROTO proto);
PyObject * get_py_data(PPROTO proto);
#endif

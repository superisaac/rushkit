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

void rtmp_proto_log(PPROTO proto, char * level, char * message) {
  if(proto->method_table->on_log) {
    proto->method_table->on_log(proto, level, message);
  } else if(STREQ(level, "ERROR") || STREQ(level, "CRITICAL")) {
    fprintf(stderr, "%s: %s\n", level, message);
  }
}

static void on_connect(PPROTO proto, READ_BUFFER * pbuffer)
{
  byte * proto_number;
  rtmp_rb_read(pbuffer, &proto_number, 1);
  assert(proto_number[0] == 3);
  proto->pkg_st = 1;
}

static int _proto_write_data(PPROTO proto, byte * data, int len) {
  proto->num_down_bytes += len;
  if(proto->method_table->on_write_data) {
    return proto->method_table->on_write_data(proto, data, len);
  }
  return 0;
}

static void on_hand_shake(PPROTO proto, READ_BUFFER * pbuffer)
{
  if(proto->tmp_buf.capacity == 0) {
    rtmp_fwb_reset(&(proto->tmp_buf), NULL, 1536);
    assert(proto->tmp_buf.capacity == 1536);
  }
  byte * shake;
  int len = rtmp_rb_read(pbuffer, &shake, rtmp_fwb_get_space(&proto->tmp_buf)); //->getSpace());
  rtmp_fwb_write(&(proto->tmp_buf), shake, len);
  if(rtmp_fwb_get_space(&proto->tmp_buf) <= 0) {
    byte * ack = ALLOC(1537);

    ack[0] = 3;
    pack_long(ack + 1, 1);
    memset(ack + 5,  '\0', 1532);

    _proto_write_data(proto, ack, 1537);
    int wl ;

    shake = rtmp_fwb_get_value(&proto->tmp_buf, &wl);
    assert(wl == 1536);

    _proto_write_data(proto, shake, wl);
    rtmp_fwb_free(&proto->tmp_buf);
    proto->pkg_st++;
  }
}

static void on_ack_back(PPROTO proto, READ_BUFFER * pbuffer)
{
  byte * data;
  int len = rtmp_rb_read(pbuffer, &data, proto->ack_back_len);
  proto->ack_back_len -= len;
  if(proto->ack_back_len <= 0) {
    proto->ack_back_len = 1536;
    proto->pkg_st++;
  }
}

void rtmp_proto_feed(PPROTO proto, int len)
{
  //trace_buffer(proto->readBuffer, len, ">>>", 60);
  proto->num_up_bytes += len;
  READ_BUFFER buffer;
  rtmp_rb_init(&buffer, proto->readBuffer, len);
  for(;rtmp_rb_get_space(&buffer)> 0 && buffer.need_more == 0;){
    switch(proto->pkg_st) {
    case 0:
      on_connect(proto, &buffer);
      break;
    case 1:
      on_hand_shake(proto, &buffer);
      break;
    case 2:
      on_ack_back(proto, &buffer);
      break;
    case 3:
      {
        PACKET * pac = rtmp_packet_get_packet(proto, &buffer);
        if(pac) {
          if(proto->method_table->on_packet) {
            proto->method_table->on_packet(proto, pac);
          } else {
	    rtmp_proto_log(proto, "WARN", "No packet callback");
          }
	  clock_t now = clock();
          rtmp_packet_reset(proto);
          rt_pool_reset(&proto->pool);
        }
      }
      break;
    }
  }
  if(buffer.need_more > 0) {
    proto->last_data_length = rtmp_rb_get_space(&buffer);

    assert(proto->last_data_length <= 128);

    if(proto->last_data_length > 0) {
      memcpy(proto->last_data, buffer.current, proto->last_data_length);
    }
  }
}

void rtmp_proto_feed_data(PPROTO proto, const byte * data, int len)
{
  byte * buffer;

  while(len > 0) {
    size_t cap;
    buffer = rtmp_proto_prepare_buffer(proto, &cap);
    if(len < cap) {
      cap = len;
    }
    len -= cap;
    assert(cap >= 0);
    memcpy(proto->readBuffer, data, cap);
    rtmp_proto_feed(proto, cap);
  }
}

static size_t _put_last_data(PPROTO proto)
{
  size_t mL = proto->last_data_length;

  if(proto->last_data && proto->last_data_length > 0){
    memcpy(proto->readBuffer, proto->last_data, proto->last_data_length);
  }
  proto->last_data_length = 0;
  return mL;
}

void * rtmp_proto_get_user_data(PPROTO proto) {
  return proto->user_data;
}

void rtmp_proto_set_user_data(PPROTO proto, void * user_data) {
  proto->user_data = user_data;
}

void rtmp_proto_init(PPROTO proto, RTMP_METHOD_TABLE * method_table)
{
  assert(method_table != NULL);
  proto->method_table = method_table;
  rt_pool_init(&proto->pool);
  proto->user_data = NULL;
  proto->read_chunk_size = 128;
  proto->write_chunk_size = 128;
  proto->last_data = ALLOC(sizeof(byte) * proto->read_chunk_size);
  proto->readBuffer = ALLOC(sizeof(byte) * BUFFER_SZ);
  proto->next_request_id = 1;
  proto->pkg_st = 0;
  proto->num_up_bytes = 0;
  proto->num_down_bytes = 0;
  proto->last_tm_bytes_read = clock();
  proto->ack_back_len = 1536;
  proto->last_data_length = 0;

  proto->channels_in = (CHANNEL*)ALLOCZ(64 * sizeof(CHANNEL));
  proto->channels_out = (CHANNEL*)ALLOCZ(64 * sizeof(CHANNEL));

  memset(proto->last_timers, 0, sizeof(int) * 64);

  rtmp_packet_init(proto);
  rtmp_fwb_init(&proto->tmp_buf);

  rtmp_packet_clear_out_channel(proto, 0);
  rtmp_packet_clear_out_channel(proto, 1);
  rtmp_packet_clear_out_channel(proto, 2);
  rtmp_packet_clear_out_channel(proto, 3);
}

void rtmp_proto_free(PPROTO proto)
{
  if(proto->last_data) {
    proto->last_data_length = 0;
    RELEASE(proto->last_data);
  }
  if(proto->readBuffer) {
    RELEASE(proto->readBuffer);
  }
  proto->user_data = NULL;
  RELEASE(proto->channels_in);
  RELEASE(proto->channels_out);
}

byte * rtmp_proto_prepare_buffer(PPROTO proto, size_t * plen) {
  size_t pL = _put_last_data(proto);
  assert(pL >= 0);

  *plen = BUFFER_SZ - pL;
  byte * buffer = proto->readBuffer + pL;
  return buffer;
}

int rtmp_proto_send_packet(PPROTO proto, POOL * ppool, PACKET * pac, BOOLEAN timer_relative)
{
  WRITE_BUFFER buffer;

  if(ppool == NULL) {
    ppool = &(proto->pool);
  }

  rtmp_wb_init(&buffer, ppool, 1024);
  rtmp_packet_write_packet(proto, &buffer, pac, timer_relative);
  int out_len;
  byte * out_data = rtmp_wb_get_value(&buffer, &out_len);
  assert(proto->method_table->on_write_data);
  assert(out_len > 0);
  return _proto_write_data(proto, out_data, out_len);
}

static void _default_on_log(PPROTO proto, char * level, char * message)
{
  printf("%s: %s\n", level, message);
}

void rtmp_proto_send_bytes_read(PPROTO proto)
{
  byte p[4];
  int len = pack_long(p, proto->num_up_bytes);
  PACKET br_pac;
  br_pac.channel_id = 2;
  br_pac.data = p;
  br_pac.channel.timer = 0;
  br_pac.channel.size = len;
  br_pac.channel.data_type = PAC_BYTES_READ;
  br_pac.channel.stream_id = 0;
  br_pac.dataLen = len;
  rtmp_proto_send_packet(proto, NULL, &br_pac, FALSE);
}

long rtmp_proto_call(PPROTO proto, char * method_name, int argc, AV * argv)
{
  int channel_id = 3;
  PACKET call_pac;
  call_pac.channel_id = channel_id;
  call_pac.channel.timer = rtmp_packet_get_last_timer(proto, channel_id);
  call_pac.channel.size = 0;
  call_pac.channel.data_type = PAC_INVOKE;
  call_pac.channel.stream_id = 0;

  long request_id = proto->next_request_id;
  proto->next_request_id = request_id + 1;
  int data_len;
  call_pac.data = amf_pack_call(&proto->pool, method_name, request_id,
				argc, argv, &data_len);
  call_pac.dataLen = data_len;
  rtmp_proto_send_packet(proto, NULL, &call_pac, FALSE);
  return request_id;
}

static void _default_on_packet_invoke(PPROTO proto, PACKET * pac)
{
  AP * pap = amf_ap_alloc(&proto->pool, pac->data, pac->dataLen);
  AV * method_name_v = amf_ap_read_next(pap);
  assert(method_name_v->type == AMF_STRING);
  AV * request_id_v = amf_ap_read_next(pap);
  assert(request_id_v->type == AMF_NUMBER);

  long request_id = (long)(request_id_v->value.number_t);

  if(!amf_string_equal(method_name_v, "_result")) {
    if(proto->next_request_id < request_id + 1) {
      proto->next_request_id = request_id + 1;
    }
  }

  AV * first_arg_v = amf_ap_read_next(pap);
  int argc;
  AK_ARRAY args;
  amf_ap_read_arguments(pap, &args);
  if(amf_string_equal(method_name_v, "connect")) {
    // Connect success messsage
    char * keys[] = {
      "level", "code", "description"
    };
    char * values [] = {
      "status", "NetConnection.Connect.Success", "Connection Succeeded."
    };

    AV value_elements[3];
    int i;
    for(i=0; i< 3; i++) {
      amf_new_string(&proto->pool, &value_elements[i], values[i]);
    }
    AK_TABLE_ELEM table_elems[3];
    AV result_v;
    amf_new_table_elem_list(&proto->pool, table_elems, 3, keys, value_elements);
    amf_new_hash(&proto->pool, &result_v, 3, table_elems);
    rtmp_proto_packet_return(proto, NULL, pac->channel_id, request_id, &result_v);
  } else if(proto->method_table->on_amf_call) {
    proto->method_table->on_amf_call(proto, pac->channel_id,
				     request_id,
				     method_name_v,
				     args.size,
				     args.elements);
  } else {
    rtmp_proto_log(proto, "WARN", "No amf callback");
  }
}

int rtmp_proto_packet_return(PPROTO proto, POOL * ppool, int channel_id, long request_id, AV * ret_val)
{
  if(ppool == NULL) {
    ppool = &(proto->pool);
  }

  AV arguments[4];
  amf_new_string(ppool, &arguments[0], "_result");
  amf_new_number(&arguments[1], request_id);
  arguments[2].type = AMF_NIL;  // First args;
  memcpy(&(arguments[3]), ret_val, sizeof(AV));

  PACKET result_pac;
  result_pac.channel_id = channel_id;
  result_pac.channel.timer = rtmp_packet_get_last_timer(proto, channel_id);
  result_pac.channel.size = 0;
  result_pac.channel.data_type = PAC_INVOKE;
  result_pac.channel.stream_id = 0;
  int data_len = 0;
  result_pac.data = amf_pack_arguments(ppool, 4, arguments, &data_len);
  result_pac.dataLen = data_len;

  if(data_len > 0) {
    rtmp_proto_send_packet(proto, ppool, &result_pac, FALSE);
  }
}

static void _default_on_packet_chunk_size(PPROTO proto, PACKET * pac)
{
  size_t chunk_size = unpack_int(pac->data);
  if(chunk_size >= proto->last_data_length) {
    byte * last_data = ALLOC(sizeof(byte) * chunk_size);
    memcpy(last_data, proto->last_data, proto->last_data_length);
    RELEASE(proto->last_data);
    proto->last_data = last_data;
  } else {
    rtmp_proto_log(proto, "WARN", "chunk size smaller than last data length");
  }
}

static void _default_on_packet_bytes_read(PPROTO proto, PACKET * pac)
{
  assert(pac->dataLen == 4);
  int num_bytes_read = unpack_int(pac->data);
}


static void _default_on_packet_ping(PPROTO proto, PACKET * pac)
{
  PACKET pong_pac;
  pong_pac.channel_id = pac->channel_id;
  pong_pac.channel.timer = rtmp_packet_get_last_timer(proto, pac->channel_id);
  pong_pac.channel.size = 0;
  pong_pac.channel.data_type = PAC_PING;
  pong_pac.channel.stream_id = 0;

  byte * myData = rt_pool_alloc(&(proto->pool), pac->dataLen);
  memcpy(myData, pac->data, pac->dataLen);
  pong_pac.data = myData;
  pong_pac.dataLen = pac->dataLen;
  rtmp_proto_send_packet(proto, NULL, &pong_pac, FALSE);
}

static void _default_on_packet(PPROTO proto, PACKET * pac)
{
  RTMP_METHOD_TABLE * table = proto->method_table;
  switch(pac->channel.data_type) {
  case PAC_CHUNK_SIZE:
    {
      if(table->on_packet_chunk_size) {
	table->on_packet_chunk_size(proto, pac);
      } else {
	rtmp_proto_log(proto, "WARN", "no callback on_packet_chunk_size");
      }
    }
    break;
  case PAC_INVALID:
    {
      if(table->on_packet_invalid) {
	table->on_packet_invalid(proto, pac);
      } else {
	rtmp_proto_log(proto, "WARN", "no callback on_packet_invalid");
      }
    }
    break;
  case PAC_BYTES_READ:
    {
      if(table->on_packet_bytes_read) {
	table->on_packet_bytes_read(proto, pac);
      } else {
	rtmp_proto_log(proto, "WARN", "no callback on_packet_bytes_read");
      }
    }
    break;
  case PAC_PING:
    {
      if(table->on_packet_ping) {
	table->on_packet_ping(proto, pac);
      } else {
	rtmp_proto_log(proto, "WARN", "no callback on_packet_ping");
      }
    }
    break;
  case PAC_INVOKE:
    {
      if(table->on_packet_invoke) {
	table->on_packet_invoke(proto, pac);
      } else {
	rtmp_proto_log(proto, "WARN", "no callback on_packet_invoke");
      }
    }
    break;
  case PAC_AUDIO:
    {
      if(table->on_packet_audio) {
	table->on_packet_audio(proto, pac);
      } else {
	rtmp_proto_log(proto, "WARN", "no callback on_packet_audio");
      }
    }
    break;
  case PAC_VIDEO:
    {
      if(table->on_packet_video) {
	table->on_packet_video(proto, pac);
      } else {
	rtmp_proto_log(proto, "WARN", "no callback on_packet_video");
      }
    }
    break;
  default:
    {
      char buf[128];
      sprintf(buf, "no such packet on type %d",  pac->channel.data_type);
      rtmp_proto_log(proto, "WARN", buf);
    }
    break;
  }
}

void rtmp_proto_method_table_init(RTMP_METHOD_TABLE * table) {
  memset(table, 0, sizeof(RTMP_METHOD_TABLE));
  table->on_log = _default_on_log;
  table->on_packet = _default_on_packet;
  table->on_packet_ping = _default_on_packet_ping;
  table->on_packet_chunk_size = _default_on_packet_chunk_size;
  table->on_packet_bytes_read = _default_on_packet_bytes_read;
  table->on_packet_invoke = _default_on_packet_invoke;
  table->on_log = _default_on_log;
}

CHANNEL * rtmp_proto_get_channel(PPROTO proto)
{
  assert(proto->channel_id < 64);
  CHANNEL * pchannel = &(proto->channels_in[proto->channel_id]);
  assert(pchannel->used == 1);
  return pchannel;
}


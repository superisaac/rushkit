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

void rtmp_packet_reset(PPROTO proto)
{
  proto->state = 0;
  proto->packet_mask = 0;
  proto->channel_id = 0;

  rtmp_fwb_init(&proto->data_buffer);
}

static void _init_channel(CHANNEL * pchannel)
{
  memset(pchannel, 0, sizeof(CHANNEL));
  pchannel->used = 1;
}

void rtmp_packet_init(PPROTO proto)
{
  proto->read_chunk_size = 128;
  proto->write_chunk_size = 128;
  proto->state = 0;
  proto->packet_mask = 0;
  proto->channel_id = 0;
  rtmp_fwb_init(&proto->data_buffer);

  rtmp_packet_clear_in_channel(proto, 0);
  rtmp_packet_clear_in_channel(proto, 1);
  rtmp_packet_clear_in_channel(proto, 2);
  rtmp_packet_clear_in_channel(proto, 3);
}

void _start_receive(PPROTO proto)
{
  rtmp_packet_reset(proto);
  proto->state ++;
}

PACKET *_get_r1(PPROTO proto,  READ_BUFFER * pbuffer)
{
  byte first_byte = rtmp_rb_read_e(pbuffer, 1)[0];
  proto->packet_mask = first_byte >> 6;
  proto->channel_id = first_byte & 0x3f;
  proto->state++;
  return NULL;
}

PACKET *_get_r2(PPROTO proto,  READ_BUFFER * pbuffer)
{
  if(proto->channel_id == 0){
    if(!rtmp_rb_assert_size(pbuffer, 1)) {
      return NULL;
    }
    proto->channel_id = 64 + rtmp_rb_read_byte_e(pbuffer);
  } else if(proto->channel_id == 1){
    if(!rtmp_rb_assert_size(pbuffer, 2)) {
      return NULL;
    }
    proto->channel_id = 64 + rtmp_rb_read_short_e(pbuffer);
  }
  proto->state ++;
  return NULL;
}

PACKET * new_invalid_packet(PPROTO  proto, byte * data, int dataLen) {
  PACKET * pack = (PACKET*)(rt_pool_alloc(&proto->pool, sizeof(PACKET)));
  pack->channel_id = proto->channel_id;
  CHANNEL * pchannel = rtmp_proto_get_channel(proto);
  if(pchannel) {
    pack->channel = *(pchannel);
  } else {
    memset(&pack->channel, 0, sizeof(pack->channel));
  }
  pack->channel.data_type = PAC_INVALID;
  pack->data = data;
  pack->dataLen = dataLen;
  return pack;
}


PACKET *_get_r3(PPROTO proto,  READ_BUFFER * pbuffer)
{
  assert(proto->channel_id < 64);
  proto->state ++;
  return NULL;
}

PACKET* _get_r4(PPROTO proto,  READ_BUFFER * pbuffer)
{
  CHANNEL * pchannel = rtmp_proto_get_channel(proto);
  switch(proto->packet_mask){
  case CHANNEL_NEW:
    {
      if(!rtmp_rb_assert_size(pbuffer, 11)) {
        return NULL;
      }
      pchannel->timer = rtmp_rb_read_medium_int_e(pbuffer);
      pchannel->size = rtmp_rb_read_medium_int_e(pbuffer);
      pchannel->data_type = rtmp_rb_read_byte_e(pbuffer);

      pchannel->stream_id = rtmp_rb_read_int_e(pbuffer);
    }
    break;
  case CHANNEL_SAME_SOURCE:
    {
      if(!rtmp_rb_assert_size(pbuffer, 7)){
        return NULL;
      }
      pchannel->timer = rtmp_rb_read_medium_int_e(pbuffer);
      pchannel->size = rtmp_rb_read_medium_int_e(pbuffer);
      pchannel->data_type = rtmp_rb_read_byte_e(pbuffer);
    }
    break;
  case CHANNEL_TIMER_CHANGE:
    {
      if(!rtmp_rb_assert_size(pbuffer, 3)) {
        return NULL;
      }
      pchannel->timer = rtmp_rb_read_medium_int_e(pbuffer);
    }
    break;
  case CHANNEL_CONTINUE:
    // Using the previous data channel
    break;
  }

  if(pchannel->size >= 32 * 1024) {
    int dataLen = rtmp_rb_get_space(pbuffer);
    byte * data =  rtmp_rb_read_e(pbuffer, dataLen);
    return new_invalid_packet(proto, data, dataLen);
  }

  proto->state ++;
  return NULL;
}


PACKET *_get_r5(PPROTO proto,  READ_BUFFER * pbuffer)
{
  CHANNEL * pchannel = rtmp_proto_get_channel(proto);
  if(pchannel->timer == 0xffffff) {
    if(!rtmp_rb_assert_size(pbuffer, sizeof(int))) {
      return NULL;
    }
    int unknown = rtmp_rb_read_int_e(pbuffer);
    char buf[60];
    sprintf(buf, "Unknown addcode %d\n", unknown);
    rtmp_proto_log(proto, "WARN", buf);
  }

  rtmp_fwb_reset(&(proto->data_buffer), &(proto->pool), pchannel->size);
  proto->state++;
  return NULL;
}


static inline int countHeaderBytes(int channel_id)
{

  if(channel_id <= 63) {
    return 0;
  } else if(channel_id < 320) {
    return 1;
  } else {
    return 2;
  }
}

PACKET * _get_packet(PPROTO proto)
{
  int dataLen;
  byte * data = rtmp_fwb_get_value(&proto->data_buffer, &dataLen); //->getValue(&dataLen);
  PACKET * pack = (PACKET*)(rt_pool_alloc(&proto->pool, sizeof(PACKET)));
  pack->channel = *rtmp_proto_get_channel(proto);
  pack->channel_id  = proto->channel_id;
  pack->data = data;
  pack->dataLen = dataLen;
  return pack;
}


PACKET *_get_r6(PPROTO proto,  READ_BUFFER * pbuffer)
{
  int hb = 1 + countHeaderBytes(proto->channel_id);
  CHANNEL * pchannel = rtmp_proto_get_channel(proto);
  if(pchannel->data_type > 0) {
    while(rtmp_fwb_get_space(&proto->data_buffer)>= proto->read_chunk_size + hb) {
      byte * trunk = rtmp_rb_read_e(pbuffer, proto->read_chunk_size);

      rtmp_fwb_write(&proto->data_buffer, trunk, proto->read_chunk_size);
      rtmp_rb_read_e(pbuffer, hb);
    }
    int vL = rtmp_fwb_get_space(&proto->data_buffer); //->getSpace();
    if(vL > 0) {
      byte * dv = rtmp_rb_read_e(pbuffer, vL);
      rtmp_fwb_write(&proto->data_buffer, dv, vL);
    }
  } else {
    if(!rtmp_rb_assert_size(pbuffer, 4)) {
      return NULL;
    }
    byte * data =  rtmp_rb_read_e(pbuffer, 4);
    rtmp_fwb_write(&proto->data_buffer, data, 4);
  }
  return _get_packet(proto);
}

int rtmp_packet_get_last_timer(PPROTO proto, int fn)
{
  assert(fn < 64);
  int now = (int)((double)(clock() * 1000) / (double)(CLOCKS_PER_SEC));
  int diff_time = now - proto->last_timers[fn];
  proto->last_timers[fn] = now;
  return diff_time;
}

PACKET * rtmp_packet_get_packet(PPROTO proto,  READ_BUFFER * pbuffer)
{

  PACKET *pac = NULL;
  do {
    switch(proto->state) {
    case 0:
      _start_receive(proto);
      break;
    case 1:
      pac = _get_r1(proto, pbuffer);
      break;
    case 2:
      pac = _get_r2(proto, pbuffer);
      break;
    case 3:
      pac = _get_r3(proto, pbuffer);
      break;
    case 4:
      pac = _get_r4(proto, pbuffer);
      break;
    case 5:
      pac = _get_r5(proto, pbuffer);
      break;
    case 6:
      pac = _get_r6(proto, pbuffer);
      break;
    }
  } while(!pac);
  if(pac) {
    //reset();
  }
  return pac;
}

void rtmp_packet_clear_out_channel(PPROTO proto, int channel_id)
{
  /*channel_map_t::iterator it = channels_out.find(channel_id);
    if(it != channels_out.end()) {
    channels_out.erase(it);
    }*/
  _init_channel(&(proto->channels_out[channel_id]));
}

void rtmp_packet_clear_in_channel(PPROTO proto, int channel_id)
{
  /*channel_map_t::iterator it = channels_in.find(channel_id);
  if(it != channels_in.end()) {
    channels_in.erase(it);
  }*/
  _init_channel(&(proto->channels_in[channel_id]));
}


void rtmp_packet_delete_out_channel(PPROTO proto, int channel_id)
{
  assert(channel_id < 64);
  proto->channels_out[channel_id].used = 0;
}

void rtmp_packet_delete_in_channel(PPROTO proto, int channel_id)
{
  assert(channel_id < 64);
  proto->channels_in[channel_id].used = 0;
}

int _writeHeaderBytes(PPROTO proto, WRITE_BUFFER * pbuffer, unsigned int packet_mask , int channel_id)
{
  if(proto->channel_id >= 64 && proto->channel_id <= 255) {
    rtmp_wb_write_byte(pbuffer, packet_mask << 6);
    rtmp_wb_write_byte(pbuffer, proto->channel_id - 64);
    return 2;
  } else if(proto->channel_id > 255) {
    rtmp_wb_write_byte(pbuffer, (packet_mask<<6) + 1);
    rtmp_wb_write_short(pbuffer, proto->channel_id - 64);
    return 3;
  } else if(proto->channel_id < 2) {
    assert(FALSE);
    return 0;
  } else {
    rtmp_wb_write_byte(pbuffer, (packet_mask << 6) + proto->channel_id);
    return 1;
  }
}

int rtmp_packet_write_packet(PPROTO proto, WRITE_BUFFER * pbuffer, PACKET * pac, BOOLEAN timer_relative)
{
  pac->channel.size = pac->dataLen;

  unsigned int packet_mask = 0;

  if(proto->channels_out[pac->channel_id].used == 0) {
    CHANNEL channel;
    _init_channel(&channel);
    proto->channels_out[pac->channel_id] = channel;
    proto->channels_out[pac->channel_id].used = 1;
  } else {
    CHANNEL * pchannel = &(proto->channels_out[pac->channel_id]);

    if(pchannel->stream_id == pac->channel.stream_id){
      packet_mask++;
    }
    if(pchannel->data_type == pac->channel.data_type
       && pchannel->size == pac->channel.size){
      packet_mask ++;
      if(pchannel->timer == pac->channel.timer){
	packet_mask ++;
      }
    }
  }
  CHANNEL * pchannel = &(proto->channels_out[pac->channel_id]);

  _writeHeaderBytes(proto, pbuffer, packet_mask, pac->channel_id);

  /*int t_f = pac->channel_id;

  if(t_f >= 64 && t_f <= 255) {
    rtmp_rb_write_byte(packet_mask << 6);
    buffer->write_byte(t_f);
  } else if(t_f > 255) {
    buffer->write_byte((packet_mask + 1)<<6);
    buffer->write_short(t_f);
  } else if(t_f < 2) {
    assert(FALSE);
  } else {
    buffer->write_byte((packet_mask << 6) + t_f);
    }*/

  double dtime = pac->channel.timer;
  if(timer_relative) {
    dtime = pac->channel.timer - pchannel->timer;
  }
  //dtime *= 1000;

  if(packet_mask == 2) {
    pchannel->timer = pac->channel.timer;
    rtmp_wb_write_medium_int(pbuffer, (int)dtime);
  } else if(packet_mask == 1) {
    pchannel->timer = pac->channel.timer;
    pchannel->data_type = pac->channel.data_type;
    pchannel->size = pac->channel.size;

    rtmp_wb_write_medium_int(pbuffer, (int)dtime);
    rtmp_wb_write_medium_int(pbuffer, pchannel->size);
    rtmp_wb_write_byte(pbuffer, pchannel->data_type);
  } else if(packet_mask == 0) {
    pchannel->timer = pac->channel.timer;
    pchannel->data_type = pac->channel.data_type;
    pchannel->size = pac->channel.size;
    pchannel->stream_id = pac->channel.stream_id;

    rtmp_wb_write_medium_int(pbuffer, (int)dtime);
    rtmp_wb_write_medium_int(pbuffer, pchannel->size);
    rtmp_wb_write_byte(pbuffer, pchannel->data_type);
    //buffer->write_reverse_int(pchannel->stream_id);
    rtmp_wb_write_reverse_int(pbuffer, pchannel->stream_id);
  }

  int rL = pac->dataLen;
  byte * p = pac->data;
  //byte ft  = 0xc0 + pac->channel_id;
  int write_chunk_size = proto->write_chunk_size;
  while(rL > write_chunk_size){
    rtmp_wb_write(pbuffer, p, write_chunk_size);
    _writeHeaderBytes(proto, pbuffer, CHANNEL_CONTINUE, pac->channel_id);
    p += write_chunk_size;
    rL -= write_chunk_size;
  }
  if(rL > 0) {
    rtmp_wb_write(pbuffer,p, rL);
  }
  return rtmp_wb_tell(pbuffer);
}




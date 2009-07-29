#include "rtmp_proto.h"

void rtmp_packet_reset(PPROTO proto)
{
  proto->state = 0;
  proto->packet_mask = 0;
  proto->channelId = 0;

  rtmp_fwb_init(&proto->data_buffer);
}


static void _init_channel(CHANNEL * pchannel)
{
  memset(pchannel, 0, sizeof(CHANNEL));
  pchannel->used = 1;
}

void rtmp_packet_init(PPROTO proto)
{
  proto->readChunkSize = 128;
  proto->writeChunkSize = 128;
  proto->state = 0;
  proto->packet_mask = 0;
  proto->channelId = 0;
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
  proto->channelId = first_byte & 0x3f;
  proto->state++;
  return NULL;    
}

PACKET *_get_r2(PPROTO proto,  READ_BUFFER * pbuffer)
{
  if(proto->channelId == 0){
    if(!rtmp_rb_assert_size(pbuffer, 1)) {
      return NULL;
    }
    proto->channelId = 64 + rtmp_rb_read_byte_e(pbuffer);
  } else if(proto->channelId == 1){
    if(!rtmp_rb_assert_size(pbuffer, 2)) {
      return NULL;
    }
    proto->channelId = 64 + rtmp_rb_read_short_e(pbuffer);
  } 
  proto->state ++;
  return NULL;
}

PACKET * new_invalid_packet(PPROTO  proto, byte * data, int dataLen) {
  PACKET * pack = (PACKET*)(rt_pool_alloc(&proto->pool, sizeof(PACKET)));
  pack->channelId = proto->channelId;
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
  //if(proto->channels_in.find(proto->channelId) == proto->channels_in.end()) {
  /*if(proto->channels_in[proto->channelId].used == 0) {
    int dataLen = rtmp_rb_get_space(pbuffer);
    if(!rtmp_rb_assert_size(pbuffer, dataLen)) {
      return NULL;
    }
    byte * data =  rtmp_rb_read_e(pbuffer, dataLen);
    return new_invalid_packet(proto, data, dataLen);
    }*/
  assert(proto->channelId < 64);
  proto->state ++;
  //proto->pchannel = &(proto->channels_in[proto->channelId]);
  //proto->pchannel->used = 1;
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

      pchannel->streamId = rtmp_rb_read_int_e(pbuffer);
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

  //assert(pchannel->size < 32 * 1024);

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


static inline int countHeaderBytes(int channelId)
{
  
  if(channelId <= 63) {
    return 0;
  } else if(channelId < 320) {
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
  pack->channelId  = proto->channelId;
  pack->data = data;
  pack->dataLen = dataLen;
  return pack;
}


PACKET *_get_r6(PPROTO proto,  READ_BUFFER * pbuffer)
{
  int hb = 1 + countHeaderBytes(proto->channelId);
  CHANNEL * pchannel = rtmp_proto_get_channel(proto);
  if(pchannel->data_type > 0) {
    while(rtmp_fwb_get_space(&proto->data_buffer)>= proto->readChunkSize + hb) {
      byte * trunk = rtmp_rb_read_e(pbuffer, proto->readChunkSize);

      rtmp_fwb_write(&proto->data_buffer, trunk, proto->readChunkSize);
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
  //int diffTime = 0;

  //if(proto->lastTimers.find(fn) != proto->lastTimers.end()){
  
  int diff_time = now - proto->last_timers[fn];
    //}
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

void rtmp_packet_clear_out_channel(PPROTO proto, int channelId)
{
  /*channel_map_t::iterator it = channels_out.find(channelId);
    if(it != channels_out.end()) {
    channels_out.erase(it);
    }*/
  _init_channel(&(proto->channels_out[channelId]));
}

void rtmp_packet_clear_in_channel(PPROTO proto, int channelId)
{
  /*channel_map_t::iterator it = channels_in.find(channelId);
  if(it != channels_in.end()) {
    channels_in.erase(it);
  }*/
  _init_channel(&(proto->channels_in[channelId]));
}


void rtmp_packet_delete_out_channel(PPROTO proto, int channelId)
{
  assert(channelId < 64);
  proto->channels_out[channelId].used = 0;
}

void rtmp_packet_delete_in_channel(PPROTO proto, int channelId)
{
  assert(channelId < 64);
  proto->channels_in[channelId].used = 0;
}

int _writeHeaderBytes(PPROTO proto, WRITE_BUFFER * pbuffer, unsigned int packet_mask , int channelId)
{
  if(proto->channelId >= 64 && proto->channelId <= 255) {
    rtmp_wb_write_byte(pbuffer, packet_mask << 6);
    rtmp_wb_write_byte(pbuffer, proto->channelId - 64);
    return 2;
  } else if(proto->channelId > 255) {
    rtmp_wb_write_byte(pbuffer, (packet_mask<<6) + 1);
    rtmp_wb_write_short(pbuffer, proto->channelId - 64);
    return 3;
  } else if(proto->channelId < 2) {
    assert(FALSE);
    return 0;
  } else {    
    rtmp_wb_write_byte(pbuffer, (packet_mask << 6) + proto->channelId);
    return 1;
  }
}

int rtmp_packet_write_packet(PPROTO proto, WRITE_BUFFER * pbuffer, PACKET * pac, BOOLEAN timer_relative)
{
  pac->channel.size = pac->dataLen;

  unsigned int packet_mask = 0;

  if(proto->channels_out[pac->channelId].used == 0) {
    CHANNEL channel;
    _init_channel(&channel);
    proto->channels_out[pac->channelId] = channel;
    proto->channels_out[pac->channelId].used = 1;
  } else {
    CHANNEL * pchannel = &(proto->channels_out[pac->channelId]);

    if(pchannel->streamId == pac->channel.streamId){
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
  CHANNEL * pchannel = &(proto->channels_out[pac->channelId]);

  _writeHeaderBytes(proto, pbuffer, packet_mask, pac->channelId);

  /*int t_f = pac->channelId;

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
    pchannel->streamId = pac->channel.streamId;

    rtmp_wb_write_medium_int(pbuffer, (int)dtime);
    rtmp_wb_write_medium_int(pbuffer, pchannel->size);
    rtmp_wb_write_byte(pbuffer, pchannel->data_type);
    //buffer->write_reverse_int(pchannel->streamId);
    rtmp_wb_write_int(pbuffer, pchannel->streamId);
  }

  int rL = pac->dataLen;
  byte * p = pac->data;
  //byte ft  = 0xc0 + pac->channelId;
  int write_chunk_size = proto->writeChunkSize;
  while(rL > write_chunk_size){
    rtmp_wb_write(pbuffer, p, write_chunk_size);
    _writeHeaderBytes(proto, pbuffer, CHANNEL_CONTINUE, pac->channelId);
    p += write_chunk_size;
    rL -= write_chunk_size;
  }
  if(rL > 0) {
    rtmp_wb_write(pbuffer,p, rL);
  }
  return rtmp_wb_tell(pbuffer);
}




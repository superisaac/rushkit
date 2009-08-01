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

#include <iostream>
#include "connection.h"

using namespace std;

int Connection::nextConnectionId = 1;
map<int, Connection*> Connection::connectionMap;

static RTMP_METHOD_TABLE method_table;

static int _writeData(PPROTO proto, byte * data, int len) {
  Connection * conn = (Connection *)(rtmp_proto_get_user_data(proto));
  return conn->writeData(data, len);
}

static void _onInvokePacket(PPROTO proto, PACKET * pac)
{
  Connection * conn = (Connection*)(rtmp_proto_get_user_data(proto));
  cout << "on invoke packet " << endl;
}

static void _on_amf_call(PPROTO proto, int channelId, double request_id, AV * method_name_v, int argc, AV * argv)
{
  if(amf_string_equal(method_name_v, "add")) {
    assert(argc == 2);
    double v1 = argv[0].value.number_t;
    double v2 = argv[1].value.number_t;
    AV result_v;
    amf_new_number(&result_v, v1 + v2);
    rtmp_proto_packet_return(proto, NULL, channelId, request_id, &result_v);

    AV word;
    amf_new_string(&proto->pool, &word, "hihi");
    rtmp_proto_call(proto, "greeting", 1, &word);
  }
}
int 
Connection::writeData(byte * data, int len)
{
  //nDownBytes += len;
  //trace_buffer(data, len, "<<<", 160);
  return write(fd, (char*)data, len);
}

void
Connection::initMethodTable() {
  rtmp_proto_method_table_init(&method_table);
  method_table.on_write_data = _writeData;
  method_table.on_amf_call = _on_amf_call;
  //method_table.on_packet_invoke = _onInvokePacket;
}

struct event Connection::bwev;

static void
_stat_callback(int fd, short why, void *arg)
{
  Connection::statBandWidth();
}

Connection *
Connection::get(int connId)
{
  if(connectionMap.find(connId) != connectionMap.end()) {
    return connectionMap[connId];
  } else {
    return NULL;
  }
}

Connection::Connection (int client_fd) 
{
  fd = client_fd;
  fcId = 1;
  conn_st = CONN_ST_WORKING;
  //lastData = new byte[proto.packFactory.readChunkSize]; LOGA(lastData);

  ///lastDataLen = 0;

  connId = nextConnectionId++;
  connectionMap[connId] = this;
  brev = NULL;

  rtmp_proto_init(&proto, &method_table);
  rtmp_proto_set_user_data(&proto, this);
}

Connection::~Connection () 
{

  rtmp_proto_free(&proto);
  connectionMap.erase(connId);
  if(brev) {
    evtimer_del(brev);
    brev = NULL;
  }
  close();
}

int 
Connection::receiveData(int fd)
{
  size_t capacity;
  byte * buffer = rtmp_proto_prepare_buffer(&proto, &capacity);  


  int len = read(fd, (char*)buffer, capacity);
  assert(len <= capacity);
  if(len >0 ){
    rtmp_proto_feed(&proto, len);
  }
  return len;
}

void
Connection::onData(const byte * data, int len)
{
  rtmp_proto_feed_data(&proto, data, len);
}

void
Connection::close()
{
  if(conn_st != CONN_ST_CLOSED) {
    event_del(&ev);
    conn_st = CONN_ST_CLOSED;
  }

}


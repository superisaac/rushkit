#ifndef _CONNECTION_H__
#define _CONNECTION_H__

#include <ctype.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>

#ifdef WINDOWS
#include <windows.h>
#include <winsock2.h>

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <err.h>
#endif

#include <errno.h>
#include <fcntl.h>

/* Required by event.h. */
#include <sys/time.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Libevent. */
#include <event.h>
#include <iostream>
#include <fstream>
#include <assert.h>

#include <map>
#include <event.h>
#include "rtmp_proto.h"

#define CONN_ST_WORKING 1000
#define CONN_ST_CLOSED 1001

using namespace std;

class Connection
{
  static int nextConnectionId;
  static struct event bwev;
  struct event * brev;
 public:
  static map<int, Connection*> connectionMap;
  static Connection * get(int connId);
  static void statBandWidth();
  static void initMethodTable();

  RTMP_PROTO proto;
  int connId;

  int fd;
  int fcId;

  struct event ev;
  int conn_st;

  Connection(int fd);
  virtual ~Connection();
  void close();

  void onClosed() {}
  int writeData(byte * data, int len);
  void onData(const byte * data, int len);
  int receiveData(int fd);
};

#endif //_CONNECTION_H__

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

#include "connection.h"
#include "signal.h"

#define DAEMON_PORT 1935

int
setsockflag(int fd)
{

#ifndef WINDOWS
  int flags;
  flags = fcntl(fd, F_GETFL);
  if (flags < 0) {
    return flags;
  }

  // Non blocking
  flags |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) < 0){
    std::cerr << "set non blocking failed" << std::endl;
    return -1;
  }
#else
  u_long mode = 1;
  ioctlsocket(fd, FIONBIO, &mode);
#endif
  int val = 1;
  // Reuse addr
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&val, sizeof(val)) < 0 ){
    return -1;
  }

  return 0;
}

void
on_rtmp_read(int fd, short ev, void *arg)
{
  Connection* client = (Connection *) arg;
  int len = client->receiveData(fd);
  if(len <= 0){
    client->onClosed();
    cout << "empty data received " << endl;
    delete client;
  }
}
void
on_accept(int fd, short ev, void *arg)
{
  int client_fd;
  struct sockaddr_in client_addr;
#ifdef WINDOWS
  int client_len = sizeof(client_addr);
#else
  socklen_t client_len = sizeof(client_addr);
#endif

  client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
  if (client_fd < 0) {
    warn("accept failed");
    return;
  }

  /* Set the client socket to non-blocking mode. */
  if (setsockflag(client_fd) < 0) {
    warn("failed to set client socket non-blocking");
  }
  Connection * client = new Connection(client_fd);
  event_set(&client->ev, client_fd, EV_READ|EV_PERSIST, on_rtmp_read, client);
  event_add(&client->ev, NULL);
}


struct event term_ev;
void on_interrupt(int fd, short ev, void * arg)
{
    cout << "Program exits normally" << endl;

  signal_del(&term_ev);
  exit(0);
}

void
greeting()
{
    cout << "Report bugs to superisaac.ke@gmail.com" << endl;
}

void init_daemon(int port, struct event * pev_accept, void (accept_callback)(int, short, void*))
{
  int daemon_fd;
  struct sockaddr_in daemon_addr;

  daemon_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (daemon_fd < 0) {
    //aerror(1, "listen failed");
    cout << "listen failed" << endl;
  }
  memset(&daemon_addr, 0, sizeof(daemon_addr));
  daemon_addr.sin_family = AF_INET;
  daemon_addr.sin_addr.s_addr = INADDR_ANY;
  daemon_addr.sin_port = htons(port); //SERVER_PORT);

  /* Set the socket to non-blocking, this is essential in event
   * based programming with libevent. */
  if (setsockflag(daemon_fd) < 0) {
    //aerror(1, "failed to set server socket to non-blocking");
    cerr << "failed to set server socket to non-blocking" << endl;
  }

  if (bind(daemon_fd, (struct sockaddr *)&daemon_addr,
	   sizeof(daemon_addr)) < 0) {
    ///aerror(1, "bind address failed");
    cerr << "bind address failed" << endl;
  }
  if (listen(daemon_fd, 5) < 0) {
    //aerror(1, "listen telnet failed");
    cerr << "listen telnet failed " << endl;
  }
  int reuseaddr_on = 1;
  setsockopt(daemon_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseaddr_on,
	     sizeof(reuseaddr_on));
  event_set(pev_accept, daemon_fd, EV_READ|EV_PERSIST, accept_callback, NULL);
  event_add(pev_accept, NULL);
}

int main(int argc, char **argv)
{
#ifndef WINDOWS
  FILE * pidfile = fopen("/tmp/flashhub.pid", "w");
  fprintf(pidfile, "%d", getpid());
  fclose(pidfile);

  signal(SIGPIPE,SIG_IGN);
#endif

#ifdef WINDOWS
 WORD wVersionRequested;
 WSADATA wsaData;
 int err;
 wVersionRequested = MAKEWORD( 2, 2 );
 err = WSAStartup( wVersionRequested, &wsaData );
 if ( err != 0 ) {
   return -1;
 }
#endif
 probe_small_endian();
  /* Initialize libevent. */
  event_init();
  Connection::initMethodTable();
  /* signaler handlers */
  signal_set(&term_ev, SIGINT, on_interrupt, NULL);
  signal_add(&term_ev, NULL);


  // Prepare listen socket
  struct event ev_accept;
  init_daemon(DAEMON_PORT, &ev_accept, on_accept);

  greeting();
  event_dispatch();
  return 0;
}


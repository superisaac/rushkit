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
    //event_del(&client->ev);
    cout << "empty data received " << endl;
     LOGD(client); delete client;
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

  //struct event * ev_switch = (struct event*)calloc(1, sizeof(struct event)); LOGA(ev_switch);

  client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
  //std::cout << "accept socket " << client_fd << " " <<fd << std::endl;
  if (client_fd < 0) {
    warn("accept failed");
    return;
  }

  /* Set the client socket to non-blocking mode. */
  if (setsockflag(client_fd) < 0) {
    warn("failed to set client socket non-blocking");
  }
  //switch_proto(client_fd);
  Connection * client = new Connection(client_fd); LOGA(client);
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
  
  //Connection::sendBytesRead();
  event_dispatch();
  return 0;
}


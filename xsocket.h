#ifndef XSOCKET_H
#define XSOCKET_H

#include "types.h"

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef int socklen_t;
  #define XSOCKET_INVALID INVALID_SOCKET
  #define XSOCKET_ERROR SOCKET_ERROR
  #define xsocket_close_fd(s) closesocket(s)
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/time.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <errno.h>
  typedef int SOCKET;
  #define XSOCKET_INVALID (-1)
  #define XSOCKET_ERROR (-1)
  #define xsocket_close_fd(s) close(s)
#endif

#endif /* XSOCKET_H */

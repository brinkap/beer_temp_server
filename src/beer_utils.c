#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include "beer_utils.h"

int make_socket (uint16_t port) {
  int sock;
  struct sockaddr_in name;

  /* Create the socket. */
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror ("socket");
    return sock;
  }

  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = INADDR_ANY;
  if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
  {
    perror ("bind");
  }

  return sock;
}

int beer_save_data_header(FILE * fd){
  assert(NULL != fd);
  fprintf(fd, "Time[Epoch, sec], T1[C], T2[C]\n");
}

int beer_save_data_raw(FILE * fd, beer_message_t msg){
  assert(NULL != fd);
  fprintf(fd, "%f, %f, %f\n", msg.time, msg.t1, msg.t2);
}

#ifndef __beer_server_h__
#define __beer_server_h__

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdarg.h>
#include "beer_types.h"
#include "mosquitto.h"

#define MAX_MSG_SIZE  4098 
#define MOSQ_MSG_SIZE 128
#define MOSQ_PUB_PERIOD 5.0

typedef struct{
  // inputs
  int port;
  int enable_logging;
  int enable_mosq;

  //state
  beer_message_t beer_message;
  struct sockaddr_in serv_addr; 
  struct sockaddr_in cli_addr;
  int sockfd;
  socklen_t clilen;
  char buffer[MAX_MSG_SIZE];
  int bufsize;
  FILE *logfd;
  FILE *datafd;
  struct mosquitto *mosq;
  char mosq_payload[MOSQ_MSG_SIZE];
  double mosq_last_pub_time;
  int iter;

  fd_set active_fd_set;
  fd_set read_fd_set;

} beer_server_t;


int beer_server_init(beer_server_t *self, 
    int argc, char *argv[]);
int beer_server_init_wiringpi(beer_server_t *self);
int beer_server_init_mosq(beer_server_t *self);
int beer_server_init_log(beer_server_t *self);

int  beer_server_process(beer_server_t *self);
void beer_server_temp_controller(beer_server_t *self);
void beer_server_listen(beer_server_t *self);
void beer_server_log(beer_server_t *self, char *fmt, ...);
void beer_server_flush_logs(beer_server_t *self);
void beer_server_handle_new_message(beer_server_t *self, int fd);
void beer_server_mosq_publish(beer_server_t *self);

void beer_server_end(beer_server_t *self);

#endif

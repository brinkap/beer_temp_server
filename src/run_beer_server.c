#include <stdio.h>
#include <signal.h>
#include "beer_server.h"
#include "beer_types.h"
#include "unistd.h"

int quit;

void sig_handler(int signum){
  quit = 1;
  PRINT("Got a Ctrl+c");
}

int main(int argc, char *argv[])
{

  quit = 0;
  signal(SIGINT, sig_handler);

  beer_server_t server;
  if(!beer_server_init(&server, argc, argv)){
    PRINT("Could not initialize server");
    return 0;
  }

  //start_pthread();

  while(!quit){
    if(!beer_server_process(&server)){
      break;
    }
    beer_server_listen(&server);
    beer_server_flush_logs(&server);
    usleep(5e5);// sleep
  }

  PRINT("Cleaning up server now...");
  beer_server_end(&server);

  return 0; 
}

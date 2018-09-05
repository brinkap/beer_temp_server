#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <math.h>
#include <openssl/sha.h>
#if HAVE_WIRINGPI
  #include "wiringPi.h"
#endif

#include "base64.h"
#include "beer_server.h"
#include "beer_utils.h"

int beer_server_init(beer_server_t *self, 
    int argc, char *argv[]){

  // Parse inputs
  if(argc != 3){
    PRINT("Error 3 arguments required");
    PRINT(" <port_no> server port e.g. 8080");
    PRINT(" <enable_logging> saves to /var/log/beer/");
    return 0;
  }

  self->port = atoi(argv[1]);
  self->enable_logging = atoi(argv[2]);
  self->beer_message.setpoint = 23; // deg c
  self->beer_message.manual_override = 0;
  self->beer_message.heater_signal = 0; // start off
  self->beer_message.cooler_signal = 0; // start off

  // Start Socket 
  self->sockfd = make_socket(self->port);
  if(self->sockfd < 0){
    return 0;
  }
  PRINT("Waiting for first connection");
  if(listen(self->sockfd, 5) < 0){
    perror("listen");
    return 0;
  }

  FD_ZERO(&self->active_fd_set);
  FD_ZERO(&self->handshake_complete);
  FD_SET(self->sockfd, &self->active_fd_set);

  // Start logging
  if(self->enable_logging){
    time_t t = time(NULL);
    struct tm stm = *localtime(&t);
    char log_file[255];
    snprintf(log_file, 255, "/var/log/beer/beer_log_%d_%d_%d_%d.txt", 
        stm.tm_mday, stm.tm_hour, stm.tm_min, stm.tm_sec);
    self->logfd = fopen(log_file, "w");
    if(!self->logfd){
      PRINT("Could not open log file %s, returning", log_file);
      return 0;
    }

    struct timespec tm;
    clock_gettime(CLOCK_MONOTONIC, &tm);
    fprintf(self->logfd, "%f: Beer server started\n",
        tm.tv_sec + 1.0e-9*tm.tv_nsec);
    fprintf(self->logfd, "%f: Port no: %d\n", 
        tm.tv_sec + 1.0e-9*tm.tv_nsec,
        self->port);

    char data_file[255];
    snprintf(data_file, 255, "/var/log/beer/beer_data_%d_%d_%d_%d.csv", 
        stm.tm_mday, stm.tm_hour, stm.tm_min, stm.tm_sec);
    self->datafd = fopen(data_file, "w");
    if(!self->datafd){
      PRINT("Could not open data file %s, returning", data_file);
      return 0;
    }
    beer_save_data_header(self->datafd);

    // setup WiringPi
    #if HAVE_WIRINGPI
      if(wiringPiSetup()==-1){
        PRINT("Could not configure wiringPi");
        return 0;
      }
    #endif 

    PRINT("Correctly setup logging files");
  }

  PRINT("Started Server on %d, logging is %d", 
      self->port, self->enable_logging);
  self->iter = 0;
  return 1;

}

int beer_server_process(beer_server_t *self){
  struct timespec tm;
  clock_gettime(CLOCK_MONOTONIC, &tm);
  self->beer_message.time = tm.tv_sec + 1.0e-9*tm.tv_nsec;

  // dummy data for now
  if(self->iter++ %10 == 0)
    PRINT("spinning %d", self->iter);

  // dummy simulated temps via Netwons law of cooling
  double dt = (self->beer_message.t2 - self->beer_message.setpoint);

  self->beer_message.t2 +=  
      K_FACTOR*(HEATER_TEMP - self->beer_message.t2)*(double)self->beer_message.heater_signal +
      K_FACTOR*(COOLER_TEMP - self->beer_message.t2)*(double)self->beer_message.cooler_signal + 
      K_FACTOR*(self->beer_message.setpoint - self->beer_message.t2);
  
  self->beer_message.t1 = self->iter;

  #if HAVE_WIRINGPI
    digitalWrite(7,self->iter%2);
  #endif

  if(self->enable_logging){
    beer_save_data_raw(self->datafd, self->beer_message);
  }
  return 1;
}

void beer_server_listen(beer_server_t *self){

  self->read_fd_set = self->active_fd_set;
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 100;
  if(select(FD_SETSIZE, &self->read_fd_set, NULL, NULL, &timeout) < 0){
    perror("select");
    return;
  }
  // Service Sockets with sent data
  int i;
  for(i=0; i<FD_SETSIZE; i++){
    if(FD_ISSET(i, &self->read_fd_set)){
      if(i==self->sockfd){
        // connection request on original socket
        size_t size = sizeof(self->cli_addr);
        int newsockfd = accept(self->sockfd, 
            (struct sockaddr *) &self->cli_addr, 
            &self->clilen);
        if (newsockfd < 0) {
          PRINT("ERROR on accept");
        }
        self->clilen = sizeof(self->cli_addr);
        PRINT("Server: New Connection from host %s, port %hd, fd %d", 
            inet_ntoa(self->cli_addr.sin_addr),
            ntohs(self->cli_addr.sin_port),
            newsockfd);
        if(self->enable_logging){
          struct timespec tm;
          clock_gettime(CLOCK_MONOTONIC, &tm);
          fprintf(self->logfd, "%f: New Connection from host %s, port %hd, fd %d\n", 
              tm.tv_sec + 1.0e-9*tm.tv_nsec,
              inet_ntoa(self->cli_addr.sin_addr),
              ntohs(self->cli_addr.sin_port),
              newsockfd);
          FD_CLR(newsockfd, &self->handshake_complete);
        }
        FD_SET(newsockfd, &self->active_fd_set);

      }else{  // Data in on preexisting connection

        self->bufsize = read(i, self->buffer, MAX_MSG_SIZE);
        if(self->bufsize < 0){
          // read error
          PRINT("bad read, Closing connection...");
          if(self->enable_logging){
            struct timespec tm;
            clock_gettime(CLOCK_MONOTONIC, &tm);
            fprintf(self->logfd, "%f: Closing connection fd %d\n", 
                tm.tv_sec + 1.0e-9*tm.tv_nsec, i);
          }
          close(i);
          FD_CLR(i, &self->active_fd_set);
        }else if(self->bufsize == 0){
          // end-of-file
          PRINT("EOF read, Closing connection...");
          if(self->enable_logging){
            struct timespec tm;
            clock_gettime(CLOCK_MONOTONIC, &tm);
            fprintf(self->logfd, "%f: Closing connection fd %d\n", 
                tm.tv_sec + 1.0e-9*tm.tv_nsec, i);
          }
          close(i);
          FD_CLR(i, &self->active_fd_set);
        }else{
          if(FD_ISSET(i, &self->handshake_complete)){
            // read data contained in buffer of len bufsize
            beer_server_handle_new_message(self, i);
          }else{
            // handshake per HTML websocket protocol
            beer_server_handle_new_websocket(self, i);
          }
        }
      } 
    } 
  }
  // Send data to active sockets
  for(i=0; i<FD_SETSIZE; i++){
    if(FD_ISSET(i, &self->active_fd_set)){
      if(i!=self->sockfd){
        int n = write(i,(void*)&self->beer_message,sizeof(beer_message_t));
        if(n < 0){
          PRINT("bad writing to socket, closing connection...");
          if(self->enable_logging){
            struct timespec tm;
            clock_gettime(CLOCK_MONOTONIC, &tm);
            fprintf(self->logfd, "%f: Closing connection fd %d\n", 
                tm.tv_sec + 1.0e-9*tm.tv_nsec, i);
          }
          close(i);
          FD_CLR(i, &self->active_fd_set);
        }
      }
    }
  }
}

void beer_server_flush_logs(beer_server_t *self){
  if(self->enable_logging){
    fflush(self->logfd);
    fflush(self->datafd);
  }
}

void beer_server_handle_new_websocket(beer_server_t *self, int fd){
  self->buffer[self->bufsize] = '\0';
  PRINT("Got First message %d \n  %s", fd, self->buffer);
  if(self->enable_logging){
    struct timespec tm;
    clock_gettime(CLOCK_MONOTONIC, &tm);
    fprintf(self->logfd, "%f: First msg from fd %d : %s\n", 
        tm.tv_sec + 1.0e-9*tm.tv_nsec,
        fd,
        self->buffer);
  }
  char *pkey = strstr(self->buffer, "Sec-WebSocket-Key:");
  if(pkey){
    PRINT("This appears to be a Websocket connection");
    char key_identifier[128];
    char key_string[128]; 
    char crap[1024];
    PRINT(" found string %s", pkey);
    char *start_loc = strchr(pkey, ':')+2;
    PRINT(" start_loc %s", start_loc);
    char *end_loc = strchr(pkey, '\r');
    int len = end_loc - start_loc;
    char candidate[1024];//= "dGhlIHNhbXBsZSBub25jZQ==";
    //int len = strlen(candidate);
    memcpy(candidate, start_loc, len);
    candidate[len] = '\0';
    PRINT(" Candidate is %s, of len %d",candidate, len); 
    size_t decoded_len;
    char* decoded_candidate = base64_decode(candidate, len, &decoded_len);
    PRINT("Decoded candidate is %s, len %d should be 16bytes", decoded_candidate, decoded_len);
    strcat(candidate, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    PRINT("Candidate after append %s", candidate);
    char digest[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*) candidate, strlen(candidate), (unsigned char*) digest);
    PRINT("SHA digest is %s", digest);
    char msg[1024] = " HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ";
    size_t outlen;
    char *base64_encoded = base64_encode(digest, strlen(digest), &outlen);
    //base64_encoded[outlen] = '\0'; // likely uneeded
    strcat(msg,base64_encoded);
    strcat(msg, "\r\n");
    strcat(msg, "Sec-WebSocket-Protocol: /\r\n");
    strcat(msg, "Sec-WebSocket-Extensions: permessage-deflate\r\n");
    strcat(msg, "\r\n");
    //int n = write(fd,(void*)msg,strlen(msg));
    PRINT("Handshaking message \n\n %s ", msg);

  }else{
    PRINT("This is not a WebSocket Connection, consider handshaking done and handle new message");
    FD_SET(fd, &self->handshake_complete);
    beer_server_handle_new_message(self, fd);
  }

}

void beer_server_handle_new_message(beer_server_t *self, int fd){
  self->buffer[self->bufsize] = '\0';
  PRINT("Got message %s", self->buffer);
  if(self->enable_logging){
    struct timespec tm;
    clock_gettime(CLOCK_MONOTONIC, &tm);
    fprintf(self->logfd, "%f: Read msg from fd %d : %s\n", 
        tm.tv_sec + 1.0e-9*tm.tv_nsec,
        fd,
        self->buffer);
  }
  char control_char;
  int value;
  int n = sscanf(self->buffer, "%c,%d", &control_char, &value);
  if(n == 2){
    switch(control_char){
      case 'S':
        PRINT("Updating setpoint to %d", value);
        self->beer_message.setpoint = value;
        break;
      case 'M':
        PRINT("Changing manual mode to  %d", value);
        self->beer_message.manual_override = value;
        break;
      case 'H':
        if(self->beer_message.manual_override){
          PRINT("heater signal changed to  %d", value);
        self->beer_message.heater_signal = value;
        }else{
          PRINT("heater signal not changed, set to manual mode (M) first");
        }
        break;
      case 'C':
        if(self->beer_message.manual_override){
          PRINT("cooler signal changed to %d", value);
          self->beer_message.cooler_signal = value;
        }else{
          PRINT("heater signal not changed, set to manual mode (M) first");
        }
        break;
      default:
        PRINT("Unhandled input message");
        break;
    }
  }

}

void beer_server_end(beer_server_t *self){
  // cleanup Socket
  close(self->sockfd);

  // cleanup logging
  if(self->enable_logging){
    fclose(self->logfd);
    fclose(self->datafd);
  }

  base64_cleanup();

  PRINT("Server Quit Successfully");
}

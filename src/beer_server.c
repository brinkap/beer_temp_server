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

#include "beer_server.h"
#include "beer_utils.h"

static double time_now(){
  struct timespec tm;
  clock_gettime(CLOCK_MONOTONIC, &tm);
  return tm.tv_sec + 1.0e-9*tm.tv_nsec;
}

int beer_server_init(beer_server_t *self, 
    int argc, char *argv[]){

  // Parse inputs
  if(argc != 4){
    PRINT("Error 3 arguments required");
    PRINT(" <port_no> server port e.g. 8080");
    PRINT(" <enable_logging> saves to /var/log/beer/");
    PRINT(" <enable_mosq> sends data to adafruit.io");
    return 0;
  }

  // initialize variables
  self->port = atoi(argv[1]);
  self->enable_logging = atoi(argv[2]);
  self->enable_mosq = atoi(argv[3]);
  self->beer_message.setpoint = 23; // deg c
  self->beer_message.manual_override = 0;
  self->beer_message.heater_signal = 0; // start off
  self->beer_message.cooler_signal = 0; // start off

  // Start Socket 
  self->sockfd = make_socket(self->port);
  if(self->sockfd < 0){
    return 0;
  }
  if(listen(self->sockfd, 5) < 0){
    perror("listen");
    return 0;
  }
  FD_ZERO(&self->active_fd_set);
  FD_SET(self->sockfd, &self->active_fd_set);

  // setup WiringPi
  if(!beer_server_init_wiringpi(self)){
    PRINT("Error, could not initialize wiringPi");
    return 0;
  }

  // Start mosq
  PRINT("before MOSQ");
  if(!beer_server_init_mosq(self)){
    PRINT("Error, could not initialize mosquitto");
    return 0;
  }
  PRINT("after MOSQ");

  // Start logging
  PRINT("before LOGGING");
  if(!beer_server_init_log(self)){
    PRINT("Error, could not initialize logs");
    return 0;
  }
  PRINT("after LOGGING");

  PRINT("Started Server on %d", self->port);
  PRINT("local logging is %d", self->enable_logging);
  PRINT("mosq  logging is %d", self->enable_mosq);

  self->iter = 0;
  return 1;
}

int beer_server_init_wiringpi(beer_server_t *self){
#if HAVE_WIRINGPI
  if(wiringPiSetup()==-1){
    PRINT("Could not configure wiringPi");
    return 0;
  }
  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
#endif 
  return 1;
}
int beer_server_init_mosq(beer_server_t *self){
  if(!self->enable_mosq){
    return 1;
  }

  self->mosq = mosquitto_new(NULL, true, NULL);
  mosquitto_username_pw_set(self->mosq, 
      "brinkap", "8294dff1a952447496372f2bed541797");

  int ret = mosquitto_connect(self->mosq, 
      "io.adafruit.com", 1883, 10);

  if( ret != MOSQ_ERR_SUCCESS){
    PRINT("Could not connect to adafruit.io site");
    mosquitto_lib_cleanup();
    return 0;
  }
  self->mosq_last_pub_time = time_now();
  printf("Successfully connected to adafruit.io!\n");
  return 1;
}



int beer_server_init_log(beer_server_t *self){
  if(!self->enable_logging){
    return 1;
  }

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
  beer_server_log(self, "Beer server started on port %d", self->port);

  char data_file[255];
  snprintf(data_file, 255, "/var/log/beer/beer_data_%d_%d_%d_%d.csv", 
      stm.tm_mday, stm.tm_hour, stm.tm_min, stm.tm_sec);
  self->datafd = fopen(data_file, "w");
  if(!self->datafd){
    PRINT("Could not open data file %s, returning", data_file);
    return 0;
  }
  beer_save_data_header(self->datafd);
  PRINT("Correctly setup logging files");
  return 1;

}

int beer_server_process(beer_server_t *self){
  struct timespec tm;
  clock_gettime(CLOCK_MONOTONIC, &tm);
  self->beer_message.time = tm.tv_sec + 1.0e-9*tm.tv_nsec;

  // dummy data for now
  if(self->iter++ %10 == 0){
    //PRINT("spinning %d", self->iter);
  }


  // dummy simulated temps via Netwons law of cooling
  double dt = (self->beer_message.t2 - self->beer_message.setpoint);
  self->beer_message.t1= 23 + 10.0*sin((double)self->iter/100.0);

  self->beer_message.t2 +=  
    K_FACTOR*(HEATER_TEMP - self->beer_message.t2)*(double)self->beer_message.heater_signal +
    K_FACTOR*(COOLER_TEMP - self->beer_message.t2)*(double)self->beer_message.cooler_signal + 
    K_FACTOR*(self->beer_message.t1- self->beer_message.t2);

  beer_server_temp_controller(self);

  

#if HAVE_WIRINGPI
  digitalWrite(0,self->beer_message.heater_signal);
  digitalWrite(1,self->beer_message.cooler_signal);
#endif

  if(self->enable_logging){
    beer_save_data_raw(self->datafd, self->beer_message);
  }
  beer_server_mosq_publish(self);
  return 1;
}

void beer_server_temp_controller(beer_server_t *self){
  if(self->beer_message.manual_override){
    return;
  }
  double dtemp = self->beer_message.t2 - 
    self->beer_message.setpoint;
  if(self->beer_message.heater_signal){
    // HEATING
    if(dtemp > 0){
      PRINT("turning off heater");
      self->beer_message.heater_signal = 0;
    }
      
  }else if(self->beer_message.cooler_signal){
    // COOLING
    if(dtemp < 0){
      PRINT("turning off cooler");
      self->beer_message.cooler_signal = 0;
    }

  }else{
    // IDLING
    if(dtemp > HOT_DEADBAND){
      PRINT("turning on cooler");
      self->beer_message.heater_signal = 0;
      self->beer_message.cooler_signal = 1;
    }else if(dtemp < -COLD_DEADBAND){
      PRINT("turning on heater");
      self->beer_message.heater_signal = 1;
      self->beer_message.cooler_signal = 0;
    }else{
      self->beer_message.heater_signal = 0;
      self->beer_message.cooler_signal = 0;
    }
  }

}


void beer_server_listen(beer_server_t *self){

  self->read_fd_set = self->active_fd_set;
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 100;
  if(select(FD_SETSIZE, &self->read_fd_set, 
        NULL, NULL, &timeout) < 0){
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
        PRINT("New Connection from host %s, port %hd, fd %d", 
            inet_ntoa(self->cli_addr.sin_addr),
            ntohs(self->cli_addr.sin_port), newsockfd);
        beer_server_log(self, 
            "New Connection from host %s, port %hd, fd %d", 
            inet_ntoa(self->cli_addr.sin_addr),
            ntohs(self->cli_addr.sin_port), newsockfd);
        FD_SET(newsockfd, &self->active_fd_set);

      }else{  // Data in on preexisting connection

        self->bufsize = read(i, self->buffer, MAX_MSG_SIZE);
        if(self->bufsize < 0){
          // read error
          PRINT("bad read, Closing connection...");
          beer_server_log(self, "Bad read, Closing connection fd %d", i);
          close(i);
          FD_CLR(i, &self->active_fd_set);
        }else if(self->bufsize == 0){
          // end-of-file
          PRINT("EOF read, Closing connection...");
          beer_server_log(self, "EOF, Closing connection fd %d", i);
          close(i);
          FD_CLR(i, &self->active_fd_set);
        }else{
          beer_server_handle_new_message(self, i);
        }
      } 
    } 
  }
  // Send data to active sockets
  for(i=0; i<FD_SETSIZE; i++){
    if(FD_ISSET(i, &self->active_fd_set)){
      if(i!=self->sockfd){
        int n = write(i,(void*)&self->beer_message,
            sizeof(beer_message_t));
        if(n < 0){
          PRINT("bad writing to socket, closing connection...");
          beer_server_log(self, "Closing connection fd %d", i);
          close(i);
          FD_CLR(i, &self->active_fd_set);
        }
      }
    }
  }
}

void beer_server_log(beer_server_t *self, char *fmt, ...){
  va_list valist;
  va_start(valist, fmt);
  char log_msg[1024];
  int n = vsnprintf(log_msg, sizeof(log_msg), fmt, valist);
  if(self->enable_logging){
    struct timespec tm;
    clock_gettime(CLOCK_MONOTONIC, &tm);
    fprintf(self->logfd, "%f: %s\n", 
        tm.tv_sec + 1.0e-9*tm.tv_nsec,
        log_msg );
  }
  va_end(valist);
}

void beer_server_flush_logs(beer_server_t *self){
  if(self->enable_logging){
    fflush(self->logfd);
    fflush(self->datafd);
  }
}


void beer_server_handle_new_message(beer_server_t *self, int fd){
  self->buffer[self->bufsize] = '\0';
  PRINT("Got message %s", self->buffer);
  beer_server_log(self, "Read msg from fd %d : %s", fd, self->buffer);

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


void beer_server_mosq_publish(beer_server_t *self){
  if(!self->enable_mosq){
    return;
  }
  double now = time_now();
  double dt = now - self->mosq_last_pub_time;
  if(dt < (double)MOSQ_PUB_PERIOD){
    // can't send data at more that 1Hz to io.adafruit
    return;
  }
  self->mosq_last_pub_time = now;

  int mid, payloadlen;

  char t1_topic[64] = "brinkap/feeds/t1";
  payloadlen = snprintf(self->mosq_payload, 128, "%lf", 
      self->beer_message.t1);
  mosquitto_publish(self->mosq, &mid, t1_topic, 
      payloadlen, self->mosq_payload, 1, false);
  //PRINT("Publishing T1 %s of len %d", 
  //    self->mosq_payload, payloadlen);

  char t2_topic[64] = "brinkap/feeds/t2";
  payloadlen = snprintf(self->mosq_payload, 128, "%lf", 
      self->beer_message.t2);
  mosquitto_publish(self->mosq, &mid, t2_topic, 
      payloadlen, self->mosq_payload, 1, false);
  //PRINT("Publishing T2 %s of len %d", 
  //    self->mosq_payload, payloadlen);

  mosquitto_loop(self->mosq, 10, 4);

}

void beer_server_end(beer_server_t *self){
  // cleanup Socket
  close(self->sockfd);

  // cleanup logging
  if(self->enable_logging){
    fclose(self->logfd);
    fclose(self->datafd);
  }

  if(self->enable_mosq){
    mosquitto_lib_cleanup();
  }
  PRINT("Server Quit Successfully");
}

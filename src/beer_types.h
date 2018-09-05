#ifndef __beer_types_h__
#define __beer_types_h__

#include <stdio.h>


#define PRINT(a, args...) printf("%s:%d " a "\n", __FILE__, __LINE__, ##args)

typedef struct{
  double time;
  double t1;
  double t2;
  double setpoint;
  int manual_override;
  int heater_signal;
  int cooler_signal;
} beer_message_t;

#define BEER_MAX_NUM_SERVERS 5

#define HEATER_TEMP 40
#define COOLER_TEMP 5
#define K_FACTOR 0.05



#endif

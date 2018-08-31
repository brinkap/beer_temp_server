#ifndef __beer_types_h__
#define __beer_types_h__

#include <stdio.h>


#define PRINT(a, args...) printf("%s:%d " a "\n", __FILE__, __LINE__, ##args)

typedef struct{
  double time;
  double t1;
  double t2;
} beer_message_t;

#define BEER_MAX_NUM_SERVERS 5



#endif

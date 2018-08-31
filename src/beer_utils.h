#ifndef __beer_utils_h__
#define __beer_utils_h__

#include <stdlib.h>
#include "beer_types.h"

int make_socket (uint16_t port);
int beer_save_data_header(FILE * fd);
int beer_save_data_raw(FILE * fd, beer_message_t msg);


#endif

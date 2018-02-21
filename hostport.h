#ifndef HOSTPORT_H
#define HOSTPORT_H

#include <stdint.h>

typedef struct __attribute__ ((packed)) {

  uint8_t ip_address[4];
  uint16_t port;
  
} hostport_t;

int fill_hostport(hostport_t *hp, char *str);

#endif

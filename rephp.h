#ifndef REPHP_H
#define REPHP_H

#include <stdint.h>

#include "hostport.h"

typedef struct __attribute__ ((packed)) {

  uint16_t rep;

  hostport_t hp;
  
} rephp_t;

long int upto_targetcount(rephp_t *rephps, long int num_reps);

rephp_t *find_targetmatch(rephp_t *rephps, long int num_reps, hostport_t hp_match);

#endif

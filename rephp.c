
#include <string.h>

#include "rep.h"

#include "rephp.h"

long int upto_targetcount(rephp_t *rephps, long int num_reps) {

  long int repno;

  long int count = 0;

  uint16_t rep_match = htons(RP_HOST);
  
  for (repno = 0; repno < num_reps && rephps[repno].rep == rep_match; repno++) {

    count++;

  }

  return count;
	 
}

rephp_t *find_targetmatch(rephp_t *rephps, long int num_reps, hostport_t hp_match) {

  long int repno;

  for (repno = 0; repno < num_reps; repno++) {

    if (rephps[repno].rep == RP_TARGET) {

      if (!memcmp(&hp_match, &rephps[repno].hp, sizeof(hostport_t))) {

	return rephps + repno;

      }
	
    }

  }
    
  return NULL;

}

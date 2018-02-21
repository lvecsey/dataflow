
#include <stdio.h>

#include "hostport.h"

int fill_hostport(hostport_t *hp, char *str) {

  long int ipvals[4];
  long int portval;
  
  int retval;

  retval = sscanf(str, "%ld.%ld.%ld.%ld:%ld", ipvals+0, ipvals+1, ipvals+2, ipvals+3, &portval);
  if (retval == 5) {

    hp->ip_address[0] = ipvals[0];
    hp->ip_address[1] = ipvals[1];
    hp->ip_address[2] = ipvals[2];
    hp->ip_address[3] = ipvals[3];	
    hp->port = portval;

    return 0;
    
  }
  
  return -1;

}


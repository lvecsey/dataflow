
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "hostport.h"

int main(int argc, char *argv[]) {

  char *input_fn = argc>1 ? argv[1] : NULL;
  
  FILE *fp = input_fn != NULL ? fopen(input_fn,"r") : stdin;

  char *line = NULL;
  
  size_t len = 0;

  long int lineno = 0;

  int retval;

  hostport_t hp;

  long int ipvals[4];
  long int portval;
  
  int out_fd = 1;
  ssize_t bytes_written;

  while ((retval=getline(&line,&len,fp)) != -1) {

    if (len > 0) {

      lineno++;

      retval = sscanf(line, "%ld.%ld.%ld.%ld:%ld", ipvals+0, ipvals+1, ipvals+2, ipvals+3, &portval);
      if (retval == 5) {
	hp.ip_address[0] = ipvals[0];
	hp.ip_address[1] = ipvals[0];
	hp.ip_address[2] = ipvals[0];
	hp.ip_address[3] = ipvals[0];	
	hp.port = portval;

	bytes_written = write(out_fd, &hp, sizeof(hostport_t));
	if (bytes_written != sizeof(hostport_t)) {
	  perror("write");
	  return -1;
	}

      }
      
    }

  }

  fclose(fp);
  
  return 0;

}


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>
#include <errno.h>
#include <stdint.h>

#include <arpa/inet.h>

#include "hostport.h"

#include "rep.h"

int main(int argc, char *argv[]) {

  char *input_fn = argc>1 ? argv[1] : NULL;
  
  FILE *fp = input_fn != NULL ? fopen(input_fn,"r") : stdin;

  char *line = NULL;
  
  size_t len = 0;

  long int lineno = 0;

  int retval;

  hostport_t hp;

  int out_fd = 1;
  ssize_t bytes_written;

  uint16_t rep;

  char *host_extract;
  
  while ((retval=getline(&line,&len,fp)) != -1) {

    if (len > 0) {

      lineno++;

      if (!strncmp(line, "TARGET ", 7)) {

	rep = htons(RP_TARGET);

	bytes_written = write(out_fd, &rep, sizeof(rep));
	if (bytes_written != sizeof(rep)) {
	  perror("writre");
	  return -1;
	}

	host_extract = line + 7;

	retval = fill_hostport(&hp, host_extract);
	if (retval != 0) {
	  fprintf(stderr, "%s: Expected to extract target host and parsing failed.\n", __FUNCTION__);
	  return -1;
	}

	bytes_written = write(out_fd, &hp, sizeof(hostport_t));
	if (bytes_written != sizeof(hostport_t)) {
	  perror("write");
	  return -1;
	}
	
	continue;
	
      }
      
      host_extract = line;

      retval = fill_hostport(&hp, host_extract);
      if (retval == 0) {

	rep = htons(RP_HOST);

	bytes_written = write(out_fd, &rep, sizeof(rep));
	if (bytes_written != sizeof(rep)) {
	  perror("writre");
	  return -1;
	}
	
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

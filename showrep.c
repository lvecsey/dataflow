
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <sys/mman.h>
#include <stdint.h>

#include <arpa/inet.h>

#include "rephp.h"

#include "rep.h"

int show_rep(rephp_t *rh) {

  switch(ntohs(rh->rep)) {
  case RP_HOST:
    printf("HOST ");
    break;
  case RP_TARGET:
    printf("TARGET ");
    break;
  default:
    printf("[Unknown] ");
  }

  {
    unsigned char *ip_address = rh->hp.ip_address;
    printf("%u.%u.%u.%u:%u\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3], rh->hp.port);
  }
	   
  return 0;
  
}

int main(int argc, char *argv[]) {

  char *filename = argc>1 ? argv[1] : NULL;
  
  void *m;
  int fd;
  struct stat buf;

  rephp_t *rephps;

  long int num_reps;
  
  long int repno;
  
  int retval;
  
  fd = open(filename, O_RDWR);

  retval = fstat(fd, &buf);
  if (retval==-1) {
    perror("fstat");
    return -1;
  }
      
  m = mmap(NULL, buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (m == MAP_FAILED) {
    perror("mmap");
    return -1;
  }

  rephps = (rephp_t*) m;

  num_reps = buf.st_size / sizeof(rephp_t);
  
  for (repno = 0; repno < num_reps; repno++) {
    show_rep(rephps+repno);
  }
  
  munmap(m, buf.st_size);
  close(fd);
  
  return 0;

}

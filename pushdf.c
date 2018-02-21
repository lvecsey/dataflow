
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <string.h>

#include <sys/mman.h>

#include "hostport.h"

#include "dataflow.h"

int main(int argc, char *argv[]) {

  size_t len;

  ssize_t bytes_written;

  char upload_buffer[4096];
  ssize_t bytes_read;

  int debug = 0;

  uint32_t dfserve_cmd;
  uint32_t sizeout;
  
  char *env_REPLICATE_FN = getenv("REPLICATE_FN");

  int fd;
  void *m;
  struct stat buf;
  
  hostport_t *hostports;

  long int hostportno;

  long int num_hostports;

  int retval;
  
  if (env_REPLICATE_FN != NULL) {

    fd = open(env_REPLICATE_FN, O_RDWR);

    retval = fstat(fd, &buf);
    if (retval == -1) {
      perror("fstat");
      return -1;
    }
    
    m = mmap(NULL, buf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (m == MAP_FAILED) {
      perror("mmap");
      return -1;
    }

    dfserve_cmd = htonl(DF_REPLICATE);
  
    bytes_written = write(7, &dfserve_cmd, sizeof(dfserve_cmd));
    if (bytes_written != sizeof(dfserve_cmd)) {
      perror("write");
      return -1;
    }

    sizeout = htonl(buf.st_size);

    bytes_written = write(7, &sizeout, sizeof(sizeout));
    if (bytes_written != sizeof(sizeout)) {
      perror("write");
      return -1;
    }

    hostports = (hostport_t *) m;
    num_hostports = buf.st_size / sizeof(hostport_t);
    hostportno = 0;
    
    bytes_written = write(7, m, buf.st_size);
    if (bytes_written != buf.st_size) {
      perror("write");
      return -1;
    }
    
  }

  dfserve_cmd = htonl(DF_PUBLISH);
  
  bytes_written = write(7, &dfserve_cmd, sizeof(dfserve_cmd));
  if (bytes_written != sizeof(dfserve_cmd)) {
    perror("write");
    return -1;
  }
  
  for (;;) {

    bytes_read = read(0, upload_buffer, sizeof(buf));

    if (debug) {
      fprintf(stderr, "%s: Read %ld bytes.\n", __FUNCTION__, bytes_read);
    }
    
    if (bytes_read > 0) {
      len = bytes_read;
      bytes_written = write(7, upload_buffer, len);
      if (bytes_written != len) {
	perror("write");
	return -1;
      }
    }
    if (bytes_read <= 0) break;
  }
  
  return 0;

}

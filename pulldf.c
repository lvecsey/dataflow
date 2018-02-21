
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

#include "dataflow.h"

int main(int argc, char *argv[]) {

  char *md5hash = argc>1 ? argv[1] : NULL;

  off_t offset = argc>2 ? strtol(argv[2], NULL, 10) : 0;
  size_t count = argc>3 ? strtol(argv[3], NULL, 10) : 0;
  
  size_t len;

  ssize_t bytes_written;

  char buf[4096];
  ssize_t bytes_read;

  size_t cnt;
  
  int debug = 0;

  uint32_t direction = htonl(DF_RETRIEVE);

  uint32_t sizeout;
  uint32_t retcode;
  
  bytes_written = write(7, &direction, sizeof(direction));
  if (bytes_written != sizeof(direction)) {
    perror("write");
    return -1;
  }
  
  if (md5hash != NULL) {
    len = strlen(md5hash);
    bytes_written = write(7, md5hash, len);
    if (bytes_written != len) {
      perror("write");
      return -1;
    }
  }

  {

    uint32_t out;

    out = htonl(offset);
    bytes_written = write(7, &out, sizeof(uint32_t));
    if (bytes_written != sizeof(uint32_t)) {
      perror("write");
      return -1;
    }

    out = htonl(count);    
    bytes_written = write(7, &out, sizeof(uint32_t));
    if (bytes_written != sizeof(uint32_t)) {
      perror("write");
      return -1;
    }    
    
  }
  
  bytes_read = read(6, &sizeout, sizeof(uint32_t));
  if (bytes_read != sizeof(uint32_t)) {
    perror("read");
    return -1;
  }

  sizeout = ntohl(sizeout);
  
  for (;sizeout > 0;) {

    cnt = sizeout;
    if (cnt > sizeof(buf)) cnt = sizeof(buf);
    
    bytes_read = read(6, buf, cnt);

    if (debug) {
      fprintf(stderr, "%s: Read %ld bytes.\n", __FUNCTION__, bytes_read);
    }
    
    if (bytes_read > 0) {
      len = bytes_read;
      sizeout -= len;
      bytes_written = write(1, buf, len);
      if (bytes_written != len) {
	perror("write");
	return -1;
      }
    }
    if (bytes_read <= 0) break;
  }
 
  bytes_read = read(6, &retcode, sizeof(uint32_t));
  if (bytes_read != sizeof(uint32_t)) {
    perror("read");
    return -1;
  }

  fprintf(stderr, "%s: retcode=%x\n", __FUNCTION__, retcode);
  
  return (retcode == DF_SUCCESS) ? 0 : -1;

}

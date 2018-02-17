
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

#include "dataflow.h"

int main(int argc, char *argv[]) {

  char *md5hash = argc>1 ? argv[1] : NULL;

  size_t len;

  ssize_t bytes_written;

  char buf[4096];
  ssize_t bytes_read;

  int debug = 0;

  uint32_t direction = htonl(DF_RETRIEVE);

  bytes_written = write(7, &direction, sizeof(direction));
  if (bytes_written != sizeof(direction)) {
    perror("write");
    return -1;
  }
  
  if (md5hash != NULL) {
    len = strlen(md5hash);
    bytes_written = write(7, md5hash, len);
  }

  for (;;) {

    bytes_read = read(6, buf, sizeof(buf));

    if (debug) {
      fprintf(stderr, "%s: Read %ld bytes.\n", __FUNCTION__, bytes_read);
    }
    
    if (bytes_read > 0) {
      len = bytes_read;
      bytes_written = write(1, buf, len);
      if (bytes_written != len) {
	perror("write");
	return -1;
      }
    }
    if (bytes_read <= 0) break;
  }
 
  
  return 0;

}

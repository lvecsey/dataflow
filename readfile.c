
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int readfile(int fd, void *buf, size_t len) {

  size_t remaining;
  unsigned char *adv_p;
  ssize_t bytes_read;
  
  adv_p = buf;
  
  remaining = len;
  while (remaining > 0) {
    bytes_read = read(fd, adv_p + len - remaining, remaining);
    if (!bytes_read) break;
    if (bytes_read < 0) {
      return -1;
    }
    remaining -= bytes_read;
  }

  return len;
  
}
    
  
  

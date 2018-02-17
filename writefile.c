
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int writefile(int fd, void *buf, size_t len) {

  size_t remaining;
  unsigned char *adv_p;
  ssize_t bytes_written;
  
  adv_p = buf;
  
  remaining = len;
  while (remaining > 0) {
    bytes_written = write(fd, adv_p + len - remaining, remaining);
    if (!bytes_written) break;
    if (bytes_written < 0) {
      return -1;
    }
    remaining -= bytes_written;
  }

  return len;
  
}

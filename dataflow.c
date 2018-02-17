
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int dataflow_setfilename(char *out_filename, char *md5str) {

  int retval;

  retval = sprintf(out_filename, "%c%c%c/%s.dat", md5str[0], md5str[1], md5str[2], md5str);
  if (retval <= 0) {
    return -1;
  }
  
  return 0;

}


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

  char *env_PROTO = getenv("PROTO");

  char *env_TCPREMOTEIP;
  char *env_TCPREMOTEPORT;

  if (env_PROTO == NULL) {
    printf("%s: You need to run this from tcpclient.\n", argv[0]);
    return -1;
  }
    
  
  if (env_REPLICATE_FN != NULL) {

    hostport_t remote_host;
    
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

    env_TCPREMOTEIP = getenv("TCPREMOTEIP");
    env_TCPREMOTEPORT = getenv("TCPREMOTEPORT");  

    if (env_TCPREMOTEIP == NULL || env_TCPREMOTEPORT == NULL) {
      printf("%s: Do not have TCPREMOTEIP or TCPREMOTEPORT environment variables.\n", __FUNCTION__);
      return -1;
    }
    
    {

      long int ipvals[4];
      long int portval;
      
      retval = sscanf(env_TCPREMOTEIP, "%ld.%ld.%ld.%ld", ipvals+0, ipvals+1, ipvals+2, ipvals+3);
      if (retval!=4) {
	printf("%s: Trouble parsing remote IP.\n", __FUNCTION__);
	return -1;
      }
      
      portval = strtol(env_TCPREMOTEPORT, NULL, 10);

      remote_host.ip_address[0] = ipvals[0];
      remote_host.ip_address[1] = ipvals[1];
      remote_host.ip_address[2] = ipvals[2];
      remote_host.ip_address[3] = ipvals[3];      
      remote_host.port = htons(portval);

      bytes_written = write(7, &remote_host, sizeof(hostport_t));
      if (bytes_written != sizeof(hostport_t)) {
	perror("write");
	return -1;
      }
      
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

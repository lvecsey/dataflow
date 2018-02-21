
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>

#include <openssl/md5.h>
#include <arpa/inet.h>

#ifdef __linux__
#include <sys/sendfile.h>
#endif

#include "dataflow.h"

#include "hostport.h"

#define TEMPLATE_FN "infile_dfserve.XXXXXX"

#define MAX_HOSTPORTS 5000

int main(int argc, char *argv[]) {

  dataflow_t df;
  
  char *df_fn = "samp.df";
  FILE *fp = fopen(df_fn, "r");

  int fd;
  struct stat buf;
  void *m;

  char data[4096];
  
  char *line = NULL;
  size_t len;
  long int lineno;

  unsigned char md5hash[MD5_DIGEST_LENGTH];

  unsigned char incoming_md5str[MD5_DIGEST_LENGTH * 2];

  char dat_fn[240];
  
  ssize_t bytes_written;
  
  int retval;

  ssize_t bytes_read;

  uint32_t dfserve_cmd;

  uint32_t sizeout;
  uint32_t retcode;
  
  const long int num_rnds = 4;
  int rnd_fd = open("/dev/urandom", O_RDONLY);
  uint64_t rnds[num_rnds];

  hostport_t hostports[MAX_HOSTPORTS];

  long int hostportno = 0;
  long int num_hostports = 0;
  
  bytes_read = read(0, &dfserve_cmd, sizeof(uint32_t));
  if (bytes_read != sizeof(uint32_t)) {
    perror("read");
    return -1;
  }

  dfserve_cmd = ntohl(dfserve_cmd);

  switch(dfserve_cmd) {
  case DF_RETRIEVE:
  
    bytes_read = read(0, incoming_md5str, sizeof(incoming_md5str));
    if (bytes_read <= 0) {
      perror("read");
      return -1;
    }

    {
      char *is = incoming_md5str;
      sprintf(dat_fn, "%c%c%c/%.32s.dat", is[0], is[1], is[2], incoming_md5str);
    }
      
    fd = open(dat_fn, O_RDWR);
    retval = fstat(fd, &buf);

    sizeout = htonl(buf.st_size);
    bytes_written = write(1, &sizeout, sizeof(uint32_t));
    if (bytes_written != sizeof(uint32_t)) {
      perror("write");
      return -1;
    }
    
#ifdef __linux__
    {
	off_t offset = 0;
	bytes_written = sendfile(1, fd, &offset, buf.st_size);
      }
#else
    m = mmap(NULL, buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (m == MAP_FAILED) {
      perror("mmap");
      return -1;
    }

    len = buf.st_size;
    bytes_written = write(1, m, len);
    if (bytes_written != len) {
      perror("write");
      return -1;
    }

    munmap(m, buf.st_size);
#endif

    retcode = htonl(DF_SUCCESS);
    bytes_written = write(1, &retcode, sizeof(uint32_t));
    if (bytes_written != sizeof(uint32_t)) {
      perror("write");
      return -1;
    }
    
    fprintf(stderr, "%s: Wrote %ld bytes.\n", __FUNCTION__, bytes_written);
  
    close(fd);
  
  break;

  case DF_PUBLISH:

    {

      size_t fsize = 0;
      
      char rnd_filename[240];
      char out_filename[240];
      len = strlen(TEMPLATE_FN);
      memset(rnd_filename, 0, sizeof(rnd_filename));
      strncpy(rnd_filename, TEMPLATE_FN, len);
      fd = mkstemp(rnd_filename);
      if (fd==-1) {
	perror("mkstemp");
	return -1;
      }
      
      for (;;) {
	bytes_read = read(0, data, sizeof(data));
	if (bytes_read >= 0) {
	  len = bytes_read;
	  bytes_written = write(fd, data, len);
	  if (bytes_written != len) {
	    perror("write");
	    return -1;
	  }
	  fsize += bytes_written;
	}
	if (bytes_read <= 0) break;
      }

      m = mmap(NULL, fsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
      if (m == MAP_FAILED) {
	perror("mmap");
	return -1;
      }

      MD5(m, fsize, md5hash);    

      {
    
	char mdString[33];

	int ipos;
	
	for(ipos = 0; ipos < 16; ipos++) {
	  sprintf(&mdString[ipos*2], "%02x", (unsigned int)md5hash[ipos]);
	}
	
	fprintf(stderr, "md5 digest: %s\n", mdString);

	{
	  char subdir_name[4] = { mdString[0], mdString[1], mdString[2], 0 };
	  mode_t dirmode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH;
	  mkdir(subdir_name, dirmode);
	  dataflow_setfilename(out_filename, mdString);
	}

	{
	  mode_t filemode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	  retval = fchmod(fd, filemode);
	}
	  
	close(fd);

	retval = rename(rnd_filename, out_filename);
	if (retval == -1) {
	  perror("rename");
	  return -1;
	}
      
      }
    
    }

    break;

  case DF_REPLICATE:

    {

      bytes_read = read(0, &sizeout, sizeof(uint32_t));
      if (bytes_read != sizeof(uint32_t)) {
	perror("read");
	return -1;
      }

      sizeout = htonl(sizeout);

      if (sizeout > sizeof(hostport_t) * MAX_HOSTPORTS) {
	sizeout = sizeof(hostport_t) * MAX_HOSTPORTS;
      }

      bytes_read = read(0, hostports, sizeout);
      if (bytes_read != sizeout) {
	perror("read");
	return -1;
      }

      num_hostports = sizeout / sizeof(hostport_t);
      
    }
      
    break;
      
  }
    
  fprintf(stderr, "%s: MD5HASH contains %ld bytes.\n", __FUNCTION__, bytes_read);
  
  while ((retval = getline(&line,&len,fp)) != -1) {

    fprintf(stderr, "%s: Got line=%s\n", __FUNCTION__, line);

    lineno++;
    
  }
  
  fclose(fp);

  close(rnd_fd);
  
  return 0;

}

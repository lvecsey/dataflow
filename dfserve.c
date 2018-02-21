
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

#include <sys/types.h>
#include <sys/socket.h>

#include "dataflow.h"

#include "hostport.h"

#include "rep.h"

#include "rephp.h"

#define TEMPLATE_FN "infile_dfserve.XXXXXX"

#define MAX_HOSTPORTS 5000

typedef struct {

  int s;
  struct sockaddr_in bind_addr;
  struct sockaddr_in dest_addr;
  
} sockpack_t;

hostport_t *read_bindlist(char *bindlist_fn, long int *num_binds) {

  hostport_t *bindlist;
  
  int fd;
  struct stat buf;
  void *m;

  int retval;
  
  fd = open(bindlist_fn, O_RDWR);
  if (fd==-1) {
    perror("open");
    return NULL;
  }

  retval = fstat(fd, &buf);
  if (retval==-1) {
    perror("fstat");
    return NULL;
  }
  
  m = mmap(NULL, buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (m == MAP_FAILED) {
    perror("mmap");
    return NULL;
  }
  
  bindlist = malloc(buf.st_size);
  if (bindlist == NULL) {
    perror("malloc");
    return NULL;
  }

  memcpy(bindlist, m, buf.st_size);

  munmap(m, buf.st_size);
  close(fd);

  if (num_binds != NULL) {
    *num_binds = buf.st_size / sizeof(hostport_t);
  }
  
  return bindlist;
  
}

int write_replicateheaders(sockpack_t *sockpacks, long int num_reps, rephp_t *rephps) {

  rephp_t *base_rephps;
  
  long int repno;

  hostport_t outgoing_hosthello;
  
  long int sending_repcount;
  
  hostport_t hp_match;
  
  int s;

  uint32_t dfserve_cmd;

  ssize_t bytes_written;

  uint32_t sizeout;
  
  for (repno = 0; repno < num_reps; repno++) {
    
    s = sockpacks[repno].s;

    dfserve_cmd = htonl(DF_REPLICATE);
  
    bytes_written = write(s, &dfserve_cmd, sizeof(dfserve_cmd));
    if (bytes_written != sizeof(dfserve_cmd)) {
      perror("write");
      return -1;
    }

    memcpy(hp_match.ip_address, &sockpacks[repno].dest_addr.sin_addr, 4);
    hp_match.port = sockpacks[repno].dest_addr.sin_port;
    
    base_rephps = find_targetmatch(rephps, num_reps, hp_match);
    if (base_rephps == NULL) {
      sending_repcount = 0;
      sizeout = 0;
    }
    else {

      sending_repcount = upto_targetcount(base_rephps, rephps+num_reps - base_rephps);
      sizeout = sizeof(rephp_t) * sending_repcount;
      
    }

    outgoing_hosthello = hp_match;
    
    bytes_written = write(s, &outgoing_hosthello, sizeof(hostport_t));
    if (bytes_written != sizeof(hostport_t)) {
      perror("write");
      return -1;
    }
      
    bytes_written = write(s, &sizeout, sizeof(sizeout));
    if (bytes_written != sizeof(sizeout)) {
      perror("write");
      return -1;
    }

    if (sizeout > 0) {
    
      bytes_written = write(s, rephps, num_reps * sizeof(rephp_t));
      if (bytes_written != num_reps * sizeof(rephp_t)) {
	perror("write");
	return -1;
      }

    }

    dfserve_cmd = htonl(DF_PUBLISH);
  
    bytes_written = write(s, &dfserve_cmd, sizeof(dfserve_cmd));
    if (bytes_written != sizeof(dfserve_cmd)) {
      perror("write");
      return -1;
    }
    
  }

  return 0;

}
  
int main(int argc, char *argv[]) {

  dataflow_t df;
  
  unsigned char bindip_address[4] = { 192, 168, 1, 50 };
  
  char *bindlist_fn = argc>1 ? argv[1] : NULL;
  
  hostport_t *bindlist = NULL;

  long int bindno;
  
  long int num_binds = 0;
  
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

  rephp_t rephps[MAX_HOSTPORTS];

  long int hostportno = 0;
  long int num_reps = 0;

  int debug = 1;
  
  {
    char *env_PROTO = getenv("PROTO");
    if (env_PROTO == NULL) {
      puts("dfserve must be run from tcpserver.");
      return -1;
    }
    if (strncmp(env_PROTO, "TCP",3)) {
      puts("dfserve must be run from tcpserver.");
      return -1;
    }
  }

  for (;;) {
  
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

	sockpack_t *sockpacks;

	socklen_t addrlen = sizeof(struct sockaddr_in); 
	char rnd_filename[240];
	char out_filename[240];

	if (num_reps > 0) {

	  sockpacks = calloc(num_reps, sizeof(sockpack_t));
	  if (sockpacks==NULL) {
	    perror("calloc");
	    return -1;
	  }

	  for (hostportno = 0; hostportno < num_reps; hostportno++) {

	    sockpacks[hostportno].s = socket(AF_INET, SOCK_STREAM, 6);
	    if (sockpacks[hostportno].s == -1) {
	      perror("socket");
	      return -1;
	    }

	    memset(&sockpacks[hostportno].bind_addr, 0, sizeof(sockpacks[hostportno].bind_addr));
	    sockpacks[hostportno].bind_addr.sin_family = AF_INET;
	    sockpacks[hostportno].bind_addr.sin_port = htons(0);;
	    memcpy(&sockpacks[hostportno].bind_addr.sin_addr.s_addr, bindip_address, 4); 
	    memset(&sockpacks[hostportno].bind_addr.sin_addr.s_addr, 0, sizeof(sockpacks[hostportno].bind_addr.sin_addr.s_addr));
	    
	    retval = bind(sockpacks[hostportno].s, (const struct sockaddr *) &sockpacks[hostportno].bind_addr, addrlen);
	    if (retval == -1) {
	      perror("bind");
	      return -1;
	    }
	  
	  }

	  for (hostportno = 0; hostportno < num_reps; hostportno++) {

	    retval = connect(sockpacks[hostportno].s, (const struct sockaddr *) &sockpacks[hostportno].dest_addr, addrlen);
	    if (retval == -1) {
	      fprintf(stderr, "%s: Trouble connecting to dest_addr.\n", __FUNCTION__);
	    }
	  
	  }
		
	  retval = write_replicateheaders(sockpacks, num_reps, rephps + upto_targetcount(rephps, num_reps));
	  if (retval == -1) {
	    fprintf(stderr, "%s: Trouble writing all replicate headers.\n", __FUNCTION__);
	    return -1;
	  }
	
	}
      
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

	    for (hostportno = 0; hostportno < num_reps; hostportno++) {
	      write(sockpacks[hostportno].s, data, len);
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

	  if (num_reps > 0) {

	    for (hostportno = 0; hostportno < num_reps; hostportno++) {
	      close(sockpacks[hostportno].s);
	    }
	  
	    free(sockpacks);

	  }
	
	}
    
      }

      break;

    case DF_REPLICATE:

      {

	hostport_t incoming_hosthello;

	hostport_t target;
      
	char *env_TCPLOCALIP = getenv("TCPLOCALIP");
	char *env_TCPLOCALPORT = getenv("TCPLOCALPORT");

	if (bindlist_fn != NULL) {
	  bindlist = read_bindlist(bindlist_fn, &num_binds);
	  if (bindlist == NULL) {
	    fprintf(stderr, "%s: Trouble opening bindlist %s.\n", __FUNCTION__, bindlist_fn);
	    return -1;
	  }
	}
      
	memset(&target, 0, sizeof(target));

	if (env_TCPLOCALIP != NULL && env_TCPLOCALPORT != NULL) {

	  long int ipvals[4];

	  if (debug) {
	    fprintf(stderr, "%s: TCPLOCALIP=%s TCPLOCALPORT=%s\n", __FUNCTION__, env_TCPLOCALIP, env_TCPLOCALPORT);
	  }
	
	  retval = sscanf(env_TCPLOCALIP, "%ld.%ld.%ld.%ld", ipvals+0, ipvals+1, ipvals+2, ipvals+3);
	  if (retval!=4) {
	    printf("WARNING: Can't parse environment variable TCPLOCALIP\n");
	    return -1;
	  }

	  target.ip_address[0] = ipvals[0];
	  target.ip_address[1] = ipvals[1];
	  target.ip_address[2] = ipvals[2];
	  target.ip_address[3] = ipvals[3];	

	  target.port = strtol(env_TCPLOCALPORT,NULL,10);

	}

	bytes_read = read(0, &incoming_hosthello, sizeof(hostport_t));
	if (bytes_read != sizeof(hostport_t)) {
	  perror("read");
	  return -1;
	}

	for (bindno = 0; bindno < num_binds; bindno++) {

	  if (!memcmp(&incoming_hosthello, bindlist+bindno, sizeof(hostport_t))) break;
	
	}

	if (bindno == num_binds) {
	  fprintf(stderr, "%s: Incoming hostport hello did not match anything in our bindlist.\n", __FUNCTION__);
	  return -1;
	}
      
	bytes_read = read(0, &sizeout, sizeof(uint32_t));
	if (bytes_read != sizeof(uint32_t)) {
	  perror("read");
	  return -1;
	}

	sizeout = htonl(sizeout);

	if (sizeout > sizeof(rephp_t) * MAX_HOSTPORTS) {
	  sizeout = sizeof(rephp_t) * MAX_HOSTPORTS;
	}

	bytes_read = read(0, rephps, sizeout);
	if (bytes_read != sizeout) {
	  perror("read");
	  return -1;
	}

	num_reps = sizeout / sizeof(rephp_t);

	fprintf(stderr, "%s: num_reps=%ld\n", __FUNCTION__, num_reps);
      
      }
      
      break;
      
    }

  }
    
  close(rnd_fd);
  
  return 0;

  }

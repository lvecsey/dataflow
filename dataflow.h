#ifndef DATAFLOW_H
#define DATAFLOW_H

enum { DF_RETRIEVE, DF_PUBLISH };

typedef struct {

  size_t content_length;
  
} dataflow_t;

int dataflow_setfilename(char *out_filename, char *md5str);

#endif

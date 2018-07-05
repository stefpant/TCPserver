#ifndef FIFO_FCTS_H
#define FIFO_FCTS_H

typedef struct fifo_info{
	char* name;
	int fd;
} fifo_info;

void initNames(fifo_info **fifo,int n);

#endif

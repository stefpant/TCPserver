#ifndef PARENT_H
#define PARENT_H

#include"fifo_fcts.h"

void parentExec(fifo_info *pfifos,char** map,int  w,int lines,pid_t *pids,int sorted,int CRfd,int CWfd);

#endif

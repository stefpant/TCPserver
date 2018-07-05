#ifndef FUNCTIONS_H
#define FUNCTIONS_H

int guard(int ret,char * error,int exit_code);
int read_command(int fd,char **buf,int size);
int itol(int value);
char* myitoa(int value);
void fillZeros(int **array,int w);
int findChildPos(pid_t chid,pid_t *pids,int w);

#endif

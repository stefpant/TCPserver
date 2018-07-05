#ifndef FUNCTIONS_H
#define FUNCTIONS_H

int guard(int ret,char * error,int exit_code);
int nextWord(char* text,char** word,int start);
int read_command(int fd,char **buf,int size);
char* read_request(int fd,int size);
int analyze_request(char* request,char **req_page);
char* copy_page(char *page,int* bytes);
char* create_answer(int *curlen,char *page,int err,int* pagebytes);
int itol(int value);
char* myitoa(int value);
void fillZeros(int **array,int w);
int findChildPos(pid_t chid,pid_t *pids,int w);

#endif

#ifndef FUNCTIONS_CR_H
#define FUNCTIONS_CR_H

char* myitoa(int value);
int analyze_url(char* url,char* host,int port,char** page);
char* create_request(char* page, char* hostname, int port);
char* read_request(int fd,int size);
int read_command(int fd,char **buf,int size);
int nextWord(char* text,char** word,int start);
int save_page(char* page,char* dir,char* content);
char** search_new_pages(char* text,int start,int* finalcounter);
void createDocfile(char* root,char* name);
int isIP(char* name);

#endif

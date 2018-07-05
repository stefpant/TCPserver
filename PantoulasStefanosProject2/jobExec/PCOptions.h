#ifndef PCOPTIONS_H
#define PCOPTIONS_H

//functions used by parent childs

#include"../trie/trie.h"

typedef struct name_count_pair nc_pair;

char* getNCname(nc_pair* self);
int getNCcounter(nc_pair* self);
void decode_plus(char* buf,int *Fbytes,int *Fwords,int *Flines);
void decode_MAX(char* buffer,char** myname,int *maxcounter);
void decode_MIN(char* buffer,char** myname,int *mincounter);
nc_pair* searchWordInTrie(node* trie,char* word,int sorted,int max);
node* getWordLastNode(node* trie,char* word,int sorted);

#endif

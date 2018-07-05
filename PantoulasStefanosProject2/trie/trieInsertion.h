#ifndef TRIEINSERTION_H
#define TRIEINSERTION_H

#include"trie.h"

int nextWord(char* text,char** word,int start);
int createTrie(char** Doc,int lines,char* name,int id,node** trie,int sorted);
void insertInTrie(node** trie,char* word,int line,char *name,int id,int sorted);

#endif

#ifndef TRIE_H
#define TRIE_H

typedef struct node node;

node* nodeConstruct(char v);
void nodeDestruct(node* self);
void setValue(node* self,char v);
char getValue(node* self);
void setNext(node* self,node* next);
node* getNext(node* self);
void setPrevious(node* self,node* next);
node* getPrevious(node* self);
void setChild(node* self,node* child);
node* getChild(node* self);
void getMaxCounterEPL(node *self,char **name,int *counter);
void getMinCounterEPL(node *self,char **name,int *counter);
int getCounterEPL(node* self);
void updateEPL(node* self,int line,char *name,int id);
char* getNamesAsString(node* self);
void getNodeData(node* self, int*** info, int* infoc);

#endif

#ifndef POSTINGLIST_H
#define POSTINGLIST_H

typedef struct postingList1 postingList1;
typedef struct postingList2 postingList2;
typedef struct ePostingList ePostingList;

ePostingList* ePostingListConstuct();
void ePostingListDestruct(ePostingList* self);
void postingList1Destruct(postingList1* self);
void getMaxCounter(ePostingList *self,char **name,int *counter);
void getMinCounter(ePostingList *self,char **name,int *counter);
int getCounter(ePostingList *self);
postingList1* insertInPL(int line,char *name,int id);
void updatePL(ePostingList **self,int line,char *name,int id);
char* getNamesAsStringPL(ePostingList *self);
void getNodeDataPL(ePostingList *self, int*** info, int* infoc);

#endif

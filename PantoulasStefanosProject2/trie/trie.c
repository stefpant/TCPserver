#include<stdio.h>
#include<stdlib.h>

#include"postingList.h"
#include"trie.h"

struct node{
	char value;
	node* next;
	node* child;
	ePostingList* myPL;
};

node* nodeConstruct(char v){
	node* newNode = malloc(sizeof( node));
	newNode->value=v;
	newNode->next=NULL;
	newNode->child=NULL;
	newNode->myPL=NULL;
//	newNode->myPL=ePostingListConstuct();
	return newNode;
}

void nodeDestruct(node* self){
	if(self==NULL) return;
	ePostingListDestruct(self->myPL);
	nodeDestruct(self->next);
	nodeDestruct(self->child);
	free(self);
	self=NULL;
	return;
}

void setValue(node* self,char v){
	self->value=v;
	return ;
}

char getValue(node* self){
	return self->value;
}

void setNext(node* self,node* next){
	self->next=next;
	return ;
}

node* getNext(node* self){
	return self->next;
}

void setChild(node* self,node* child){
	self->child=child;
	return ;
}

node* getChild(node* self){
	return self->child;
}

void getMaxCounterEPL(node *self,char **name,int *counter){
	if(self == NULL) return;
	if(self->myPL==NULL) return;
	getMaxCounter(self->myPL,name,counter);
}

void getMinCounterEPL(node *self,char **name,int *counter){
	if(self == NULL) return;
	if(self->myPL==NULL) return;
	getMinCounter(self->myPL,name,counter);
}

int getCounterEPL(node* self){
	if(self == NULL) return 0;
	if(self->myPL==NULL) return 0;
	return getCounter(self->myPL);
}

void updateEPL(node* self,int line,char *name,int id){
	if(self->myPL==NULL) self->myPL=ePostingListConstuct();
	updatePL(&(self->myPL),line,name,id);
}

char* getNamesAsString(node* self){
	if(self==NULL) return NULL;
	if(self->myPL==NULL) return NULL;
	return getNamesAsStringPL(self->myPL);
}

void getNodeData(node* self, int*** info, int *infoc){
	if(self==NULL) return;
	if(self->myPL==NULL) return;
	getNodeDataPL(self->myPL,info,infoc);
}






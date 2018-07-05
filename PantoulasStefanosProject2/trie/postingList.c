#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"postingList.h"

struct ePostingList{
	int counter;//num of elements
	postingList1 *pl;//pl's first node
	postingList1 *last;//pl's last node
};

struct postingList1{//pl for files
	int counter;//how many times the word showed in this file
	int linecounter;//in how many lines it showed
	char* name;//file(full path)
	int id;//useful to point the map instead of searching maps by name
	postingList1 *next;
	postingList2 *pl;//points to first line in file(that has the word)
	postingList2 *last;//last line
//actually first-last line that has the 'word'
};

struct postingList2{//pl for lines in file
	int line;
	postingList2 *next;
};

ePostingList* ePostingListConstuct(){
	ePostingList* epl = malloc(sizeof(ePostingList));
	epl->counter=0;
	epl->pl = NULL;
	epl->last=NULL;
	return epl;
}

void ePostingListDestruct(ePostingList* self){
	if(self==NULL) return;
	postingList1Destruct(self->pl);
	self->last = NULL;
	free(self);
	return;
}

void postingList2Destruct(postingList2* self){
	if(self==NULL) return;
	postingList2Destruct(self->next);
	free(self);
	return;
}

void postingList1Destruct(postingList1* self){//recursive destruction
	if(self==NULL) return;
	postingList2Destruct(self->pl);
	postingList1Destruct(self->next);
	self->name = NULL;
	self->last = NULL;
	free(self);
	return;
}

postingList2* insertInPL2(int line){
	postingList2* pl = malloc(sizeof(postingList2));
	pl->line = line;
	pl->next = NULL;
	return pl;
}

postingList1* insertInPL(int line,char* name,int id){//creates new pl's node
	postingList1* pl = malloc(sizeof(postingList1));
	pl->name = name;//just point to real name
	pl->id = id;
	pl->counter=1;
	pl->linecounter=1;
	pl->next=NULL;
	pl->pl = insertInPL2(line);
	pl->last = pl->pl;
	return pl;
}

void getMaxCounter(ePostingList *self,char **name,int *counter){
	postingList1 *temp = self->pl;
	*name = temp->name;
	*counter = temp->counter;
	while((temp = temp->next) != NULL){
		if(temp->counter>*counter || (temp->counter==*counter && strcmp(temp->name,*name)<0)){
			*name = temp->name;
			*counter = temp->counter;
		}
	}
	return;
}

void getMinCounter(ePostingList *self,char **name,int *counter){
	postingList1 *temp = self->pl;
	*name = temp->name;
	*counter = temp->counter;
	while((temp = temp->next) != NULL){
		if(temp->counter<*counter || (temp->counter==*counter && strcmp(temp->name,*name)<0)){
			*name = temp->name;
			*counter = temp->counter;
		}
	}
	return;
}

int getCounter(ePostingList *self){
	return self->counter;
}

void updatePL(ePostingList **self,int line,char *name,int id){
	if((*self)->pl==NULL){
		(*self)->pl=insertInPL(line,name,id);
		(*self)->last = (*self)->pl;
		(*self)->counter++;//new PL node
		return;
	}
	postingList1 *temp = (*self)->last;
	if(!strcmp(temp->name,name) && (temp->last)->line>line){//that will never happen :-)
		printf("You shouldn't come here,leave me alone ~_~'\n");
		return ;
	}
	//checking name or id is the same in PL
	if(!strcmp(temp->name,name)){//now lets check pl2
		temp->counter++;//increase counter for the file
		if((temp->last)->line == line){}//do nothing
			//(temp->last)->freq++;//freq deleted
		else{//new line,create next pl2
			temp->linecounter++;
			(temp->last)->next = insertInPL2(line);
			((*self)->last)->last = (temp->last)->next;
		}
		return;
	}
	temp->next = insertInPL(line,name,id);
	(*self)->last = temp->next;
	(*self)->counter++;//new PL node
	return ;
}

void replaceSpaces(char** str){
	if(str == NULL) return;
	int len;
	if((len = strlen(*str)) == 0) return;
	for(int i=0;i<len;i++)
		if((*str)[i] == ' ') (*str)[i]='_';
	return;
}

char* getNamesAsStringPL(ePostingList *self){//self != NULL here
	if(self->pl==NULL) return NULL;
	postingList1 *temp = self->pl;
	char *name = temp->name;
	replaceSpaces(&name);
	int rescount=strlen(name)+4;
	char *tmp=" : ";
	char *res = malloc(rescount*sizeof(char));
	strcpy(res,tmp);
	strcat(res,name);
	while((temp=temp->next) != NULL){
		name = temp->name;
		replaceSpaces(&name);
		rescount+=strlen(name)+3;
		res = realloc(res,rescount*sizeof(char));
		strcat(res,tmp);
		strcat(res,name);
	}
	res[rescount-1] = '\0';
	return res;
}

void getNodeDataPL(ePostingList *self, int*** info, int* infoc){
	if(self->pl==NULL) return;
	int fcounter = self->counter;
	int flcounter = 0;
	postingList1 *temp = self->pl;
	for(int i=0;i<fcounter;i++){
		flcounter += temp->linecounter;
		temp = temp->next;
	}
	if(flcounter == 0) return;
	(*info) = malloc(flcounter*sizeof(int*));
	*infoc = flcounter;
	flcounter = 0;
	temp = self->pl;
	for(int i=0;i<fcounter;i++){
		postingList2 *pl2temp = temp->pl;
		for(int j=0;j<temp->linecounter;j++){
			(*info)[flcounter + j] = malloc(2*sizeof(int));//[fileid,line]
			(*info)[flcounter + j][0] = temp->id;
			(*info)[flcounter + j][1] = pl2temp->line;
			pl2temp = pl2temp->next;
		}
		flcounter += temp->linecounter;
		temp = temp->next;
	}
	return;
}



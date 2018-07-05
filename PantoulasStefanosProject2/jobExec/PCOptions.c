#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"../trie/trieInsertion.h"
#include"../trie/trie.h"
#include"PCOptions.h"

struct name_count_pair{
	char *name;
	int counter;
};

char* getNCname(nc_pair* self){
	return self->name;
}

int getNCcounter(nc_pair* self){
	return self->counter;
}

//buffer's form should be like "BYTES WORDS LINES"
//this function do:
//*bytes += atoi("BYTES")
//*words += atoi("WORDS")
//*lines += atoi("LINES")
//also in the end checks if buffer had that form
void decode_plus(char* buffer,int *bytes,int *words,int *lines){//for WC
	if(buffer==NULL) return;
	int start;
	char* word = NULL;
	start = nextWord(buffer,&word,0);//read bytes
	*bytes += atoi(word);
	free(word);
	word = NULL;
	start = nextWord(buffer,&word,start);
	*words += atoi(word);
	free(word);
	word = NULL;
	start = nextWord(buffer,&word,start);
	*lines += atoi(word);
	free(word);
	word = NULL;
	if(start != -1) printf("Unexpected result!(%s)\n",buffer);
	return;
}

void decode_MAX(char* buffer,char** myname,int *maxcounter){//for MAXCOUNT
	if(!strcmp(buffer,"-")) return;//no result from child
	char* name;
	char* word;
	int counter;
	int start = nextWord(buffer,&word,0);
	int name_len = strlen(word) + 1;
	name = malloc(name_len*sizeof(char));
	strcpy(name,word);
	free(word);
	while((start = nextWord(buffer,&word,start)) != -1){//name maybe has spaces
		name_len += strlen(word) + 2;
		name = realloc(name,name_len*sizeof(char));
		strcat(name," ");
		strcat(name,word);
		free(word);
	}
	counter = atoi(word);
	free(word);
	if(counter>*maxcounter || (counter==*maxcounter && strcmp(name,*myname)<0)){
		*maxcounter = counter;
		if(*myname != NULL) free(*myname);
		*myname = name;
	}
	else free(name);
}

void decode_MIN(char* buffer,char** myname,int *mincounter){//for MINCOUNT
	if(!strcmp(buffer,"-")) return;//no result from child
	char *name;
	char* word;
	int counter;	
	int start = nextWord(buffer,&word,0);
	int name_len = strlen(word) + 1;
	name = malloc(name_len*sizeof(char));
	strcpy(name,word);
	free(word);
	while((start = nextWord(buffer,&word,start)) != -1){//name maybe has spaces
		name_len += strlen(word) + 2;
		name = realloc(name,name_len*sizeof(char));
		strcat(name," ");
		strcat(name,word);
		free(word);
	}
	counter = atoi(word);
	free(word);
	if(counter<*mincounter || *mincounter==-1 || (counter==*mincounter && strcmp(name,*myname)<0)){
		*mincounter = counter;
		if(*myname != NULL) free(*myname);
		*myname = name;
	}
	else free(name);
}

nc_pair* searchWordInTrie(node* trie,char* word,int sorted,int max){//for min/maxcount
	int i=0;//character's index
	char c = word[i];
	node* temp=trie;
	int flag=1;
	//flag:if temp->next==NULL then create new next

	nc_pair* p = malloc(sizeof(nc_pair));
	p->name = NULL;//means word not exists
	p->counter = -1;

	while(1){//for all word's chars(has at least 1 char)
		while(1){//for all childs in temp's list(temp -> next -> next -> ...)
			if(c==getValue(temp)){
				flag=0;
				break;//check next char
			}
			else if((c<getValue(temp))&&sorted) break;
			if(getNext(temp)==NULL) return p;//word doesn't exists in trie :(
			temp = getNext(temp);//search until c found in trie
		}//or we reached the end of temp's list
		if(flag)//c not found in trie...exiting
			return p;

		//now temp has the 'c' value so we check its child
		if((c=word[++i])=='\0'){
			if(max)
				getMaxCounterEPL(temp,&(p->name),&(p->counter));
			else
				getMinCounterEPL(temp,&(p->name),&(p->counter));
			return p;
		}
		if(getChild(temp)==NULL) return p;//word not found
		temp=getChild(temp);//else continue with child
	}
}

//***copy from trieOptions.c from project1***
//here we need the pointer to last node of the 'word' to have
//access to its posting list
node* getWordLastNode(node* trie,char* word,int sorted){//for search - child
	int i=0;//character's index
	char c = word[i];
	node* temp=trie;
	int flag=1;
	//flag:if temp->next==NULL then create new next

	while(1){//for all word's chars(has at least 1 char)
		while(1){//for all childs in temp's list(temp -> next -> next -> ...)
			if(c==getValue(temp)){
				flag=0;
				break;//check next char
			}
			else if((c<getValue(temp))&&sorted) break;
			if(getNext(temp)==NULL) return NULL;//word doesn't exists in trie :(
			temp = getNext(temp);//search until c found in trie
		}//or we reached the end of temp's list
		if(flag)//c not found in trie...exiting
			return NULL;
		//now temp has the 'c' value so we check its child
		if((c=word[++i])=='\0'){
			if(getCounterEPL(temp)==0) return NULL;//word found but is subword in trie
			return temp;
		}
		if(getChild(temp)==NULL) return NULL;//word not found
		temp=getChild(temp);//else continue with child
	}
}



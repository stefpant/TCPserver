#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<errno.h>
#include<dirent.h>
#include<unistd.h>
#include<poll.h>
#include<sys/types.h>
#include<sys/stat.h>
int itol(int value){
	int counter=1;
	if(value<0) value*=-1;
	while((value/=10)>0) counter++;
	return counter;
}

char* myitoa(int value){
	int c=itol(value);
	char *ptr = (char *)malloc((c+1)*sizeof(char));
	ptr[c--] = '\0';
	if(value == 0) ptr[0] = '0';
	while(value){
		ptr[c--] = value%10 + '0';
		value/=10;
	}
	return ptr;
}


int analyze_url(char* url,char* host,int port,char** page){
	//url should be like 'http://host:port/page'
	//only page need to be returned
	int hostsize=strlen(host);
	if(strlen(url) < 12+hostsize) return -1;

	url[6]='\0';
	url[7+hostsize]='\0';
	url[12+hostsize]='\0';
	//now url should be like 'http:/\0host\0port\0page'
	if(strcmp(url,"http:/")) return -1;
	//if(strcmp(url+7,host)) return -1;
	char *cport=myitoa(port);
	if(strcmp(url+8+hostsize,cport)) {free(cport);return -1;}
	(*page) = malloc((strlen(url+13+hostsize)+2)*sizeof(char));
	strcpy((*page),"/");
	strcat((*page),url+13+hostsize);
	//printf("%s\n",(*page));
	free(cport);
	return 0;
}


char* create_request(char* page, char* hostname, int port){
	char *req;
	char* chport=myitoa(port);
	int size=47+strlen(page)+strlen(hostname)+strlen(chport);
	req = malloc(size*sizeof(char));
	strcpy(req,"GET ");
	strcat(req,page);
	strcat(req," HTTP/1.1\r\nHost: ");
	strcat(req,hostname);
	strcat(req,":");
	strcat(req,chport);
	strcat(req,"\r\nConnection: Closed\r\n\r\n");
	free(chport);
	return req;
}


char* read_request(int fd,int size){
	int n;
	int counter=1;
	int flag;
	char *tempbuf=NULL;
	char *buf=NULL;
	while(1){
		flag=0;
		tempbuf=calloc((size+1),sizeof(char));
		if((n=read(fd,tempbuf,size)) <= 0) break;
		if(counter == 1) flag=1;
		counter+=n;
		if(flag){
			buf = malloc((counter+1)*sizeof(char));
			strcpy(buf,tempbuf);//first time copy
		}
		else{
			buf = realloc(buf,(counter+1)*sizeof(char));
			strcat(buf,tempbuf);//then cat
		}
		free(tempbuf);
	}
	free(tempbuf);
	return buf;
}


int read_command(int fd,char **buf,int size){
	char c;
	int n;
	int counter=0;
	while(1){
		if((n=read(fd,&c,1)) < 0) return n;
		if(!n)
			break;
		if(counter == size){//then realloc 
			size *=2;
			(*buf) = realloc((*buf),size*sizeof(char));
		}
		if(c == '\0' || c == EOF || c == '\n' || c == 13){//13 is the carriege return
			read(fd,&c,1);//read the '\n'
			(*buf)[counter] = '\0';
			break;
		}
		else (*buf)[counter++] = c;
	}
	return counter+1;
}

int nextWord(char* text,char** word,int start){
	if(text==NULL) return -2;//error
	if(start>strlen(text) || start<=-1) return -2;//error or END
	while(text[start]==' ' || text[start]=='\t' || text[start]=='\n' || text[start]==13) start++;//skip ws
	//now in text[start] starts the word which we return
	if(text[start]=='\0') return -2;//or maybe we reached '\0'
	int temp=start;//if not return the word
	while(text[temp]!=' ' && text[temp]!='\t' && text[temp]!='\0' && text[temp]!=13 && text[temp]!='\n') temp++;
	if((*word = malloc((temp-start+1)*sizeof(char)))==NULL){
		printf("Allocation Error!\n");
		return -3;
	}
	strncpy(*word,text+start,temp-start);
	(*word)[temp-start] = '\0';
	while(text[temp]==' ' || text[temp]=='\t' || text[temp]=='\n' || text[temp]==13) temp++;//again skip ws
	if(text[temp]=='\0')//if after this word we reached the end
		return -1;
	return temp;//first char to check next time
}//after using the 'word' needs free!

char* getDir(char* page,char* d){
	int i=1;
	while(1){
		if(page[i++] == '/'){
			page[i-1]='\0';
			break;
		}
	}
	char* dir=malloc((strlen(page)+strlen(d)+1)*sizeof(char));
	strcpy(dir,d);
	strcat(dir,page);
	return dir;
}

int skip_header(char* text){
	int p=0;

	while(text[p]!='\n' || text[p+2]!='\n'){p++;}
	return p+3;
}

int save_page(char* page,char* dir,char* content){

	if(content[9]!='2' || content[10]!='0' || content[11]!='0') return -1;//wrong file

	char *fname=malloc((strlen(page)+strlen(dir)+2)*sizeof(char));
	strcpy(fname,dir);
	strcat(fname,"/");
	strcat(fname,page);
	char* pagedir = getDir(page,dir);
	DIR* d = opendir(pagedir);
	if(d){//dir exists
		closedir(d);
	}
	else if(errno == ENOENT){//dir doesn't exists
		mkdir(pagedir,0700);
	}
	free(pagedir);
	FILE* fp=fopen(fname,"w");
	if(!fp) {free(fname);free(pagedir);return -1;}//couldn't create file
	int start=skip_header(content);//return a pointer after a header to copy the file

	fprintf(fp,"%s",content+start);

	free(fname);
	fclose(fp);
	return start;
}

char* getURL(char* x){
	if(strlen(x)<5) return NULL;
	x[4]='\0';//should be '=' in x[4]
	if(strcmp(x,"href")) return NULL;
	int c=5;
	while(1){
		if('>' == x[c]){x[c]='\0';break;}
		else if('\0' == x[c++]) return NULL;
	}
	char* ret=malloc((strlen(x+5)+1)*sizeof(char));
	strcpy(ret,x+5);
	return ret;
}

char** search_new_pages(char* text,int start,int* finalcounter){
	char **res=malloc(sizeof(char*));
	int counter=0;
	char* word;
	char* suburl=NULL;
	while(start!=-1){
		word = NULL;
		start = nextWord(text,&word,start);//save word
		if(start<-1) break;
		if(!strcmp(word,"<a")){//maybe a first step to find new url
			free(word);
			word=NULL;
			start = nextWord(text,&word,start);
			if(start<-1) break;
			suburl=getURL(word);
			if(suburl == NULL){free(word);continue;}//not url here :-(
			if(counter++)
				res=realloc(res,(counter+1)*sizeof(char*));
			res[counter-1]=suburl;
			suburl=NULL;
		}
		free(word);
	}
	*finalcounter=counter;
	return res;
}

void createDocfile(char* root,char* name){
	FILE* fp=fopen(name,"w");
	struct dirent *d;
	DIR *dir = opendir(root);
	char* founddir=NULL;
	if(dir == NULL)//gdir not a directory
		return;
	while((d = readdir(dir)) != NULL){
		if(d->d_type != DT_DIR) continue;//d not a directory
		if(!strcmp(d->d_name,".") || !strcmp(d->d_name,"..")) continue;//ignore '.','..'
		founddir = malloc((strlen(root)+strlen(d->d_name)+2)*sizeof(char));
		strcpy(founddir,root);
		strcat(founddir,"/");
		strcat(founddir,d->d_name);
		fprintf(fp,"%s\n",founddir);
		free(founddir);
		founddir=NULL;
	}
	closedir(dir);
	fclose(fp);
}

int isIP(char *name){//ip4: xxx.xxx.xxx.xxx(maxlen=15)
	if(name==NULL) return 0;
	char c;
	int i=0;
	int dots=0,digs=0;
	while( (c=name[i++]) != '\0' ){
		if(c == '.') dots++;
		else if( (c>='0') && (c<='9')) digs++;
		else return 0;//not ip

		if((dots > 3) || (digs > 12)) return 0;
	}
	if( (dots != 3) || (digs<4) ) return 0;
	return 1;
}

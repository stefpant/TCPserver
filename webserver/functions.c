#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<unistd.h>
#include<poll.h>

//function 1
int guard(int ret,char * error,int exit_code){
	if(ret < 0){perror(error);exit(exit_code);}
	return ret;
}


//function 2
//gets a string text and a start pointer in text to tell us where to
//start searching the next word.Finds and saves this word and returns the
//'start' pointer where we will use to find the next word
//When found the last word returns -1
//if start is already -1 or we found an error then return -2 ->STOP
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

//function 3
int itol(int value){
	int counter=1;
	if(value<0) value*=-1;
	while((value/=10)>0) counter++;
	return counter;
}

//function 4
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

//function 5
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

/*
int read_request2(int fd,char **buf,int size){
	char c,prevc='-';
	int n,polln;
	int counter=0;
	clock_t start,end;
	float eltime;
	int timeout=10;//poll timeout
	int pollto;
	start = clock();
	struct pollfd pfd[1];
	pfd[0].fd=fd;
	pfd[0].events = POLLIN;
	while(1){
		end = clock();
		eltime = (float)(end-start)/CLOCKS_PER_SEC; 
		pollto = 1000*(timeout-eltime);
		polln = poll(pfd,1,pollto);
		if(polln == 0){
			if(counter > 0) counter--;
			(*buf)[counter] = '\0';
			break;
		}
		else
			if((n=read(fd,&c,1)) < 0) return n;

		if(counter == size){//then realloc 
			size *=2;
			(*buf) = realloc((*buf),size*sizeof(char));
		}
		if( c == 13 && prevc == '\n' ){//13 is the carriege return
			(*buf)[counter] = '\0';//newline here is 'cr''nl'
			break;//so for 2newlines in a row we need prevc='nl' and c='cr'
		}
		else (*buf)[counter++] = c;
		prevc = c;
	}
	return counter+1;
}*/

int analyze_request(char* request,char **req_page){
	if(request == NULL) return -1;//bad request
	int start=0;
	char *word;
	short flagGET=0,flagCON=0,flagHOST=0;//flagUA=0,flagACL=0,flagACE=0;
	while(start!=-1){
		word = NULL;
		start = nextWord(request,&word,start);//save word
		if(start == -2) break;//request was empty
		if( !strcmp(word,"GET") ){
			flagGET=1;
			start = nextWord(request,req_page,start);//save req_page
			free(word);
			word=NULL;
			start = nextWord(request,&word,start);//check protocol
			if( strcmp(word,"HTTP/1.1") ){
				free(word);
				break;
			}
		}
		else if( !strcmp(word,"Host:") ) flagHOST=1;
		//else if( !strcmp(word,"User-Agent:") ) flagUA=1;
		//else if( !strcmp(word,"Accept-Language:") ) flagACL=1;
		//else if( !strcmp(word,"Accept-Encoding:") ) flagACE=1;
		else if( !strcmp(word,"Connection:") ) flagCON=1;
		free(word);
	}

	//if(flagGET && flagCON && flagUA && flagHOST && flagACL && flagACE)
	if(flagGET && flagCON &&  flagHOST)
		return 0;//OK

	return -1;//error missing one header
}

//file 'page' exists for sure and we have permission to read
char* copy_page(char *page,int* count){
	*count=0;
	FILE *fp = fopen(page,"r");
	int maxlen = 20;
	char c;
	char* cppage = malloc(20*sizeof(char));
	while( 1 ){
		if(*count == maxlen){
			maxlen*=2;
			cppage = realloc(cppage,maxlen*sizeof(char));
		}
		c = fgetc(fp);
		if( !feof(fp) )
			cppage[*count] = c;
		else{
			cppage[*count] = '\0';
			break;
		}
		(*count)++;
	}
	fclose(fp);
	cppage = realloc(cppage,(*count + 1)*sizeof(char));
	return cppage;
}

char* create_answer(int *curlen,char *page,int err,int* pagebytes){
	time_t timer;
	char timebuffer[26];
	struct tm* tm_info;
	char* ans = malloc(200*sizeof(char));
	int flag=0;

	if( err == -1){
		strcpy(ans,"HTTP/1.1 400 Bad Request\r\n");
		flag=1;
	}
	else if(0 != access(page,F_OK)){
		strcpy(ans,"HTTP/1.1 404 Not Found\r\n");
		flag=2;
	}
	else if(0 != access(page,R_OK)){
		strcpy(ans,"HTTP/1.1 403 Forbidden\r\n");
		flag=3;
	}
	else
		strcpy(ans,"HTTP/1.1 200 OK\r\n");
	*curlen = 200;
	int bytes;
	//char* line = NULL;
	char* charbytes = NULL;

	strcat(ans,"Date: ");

	time(&timer);
	tm_info = localtime(&timer);
	strftime(timebuffer, 26, "%a, %d-%m-%Y %H:%M:%S", tm_info);

	strcat(ans,timebuffer);
	strcat(ans," GMT\r\n");
	strcat(ans,"Server: webserver/1.0.0 (Ubuntu64)\r\n");
	strcat(ans,"Content-Length: ");
	char* cppage = NULL;
	if( !flag )
		cppage = copy_page(page,&bytes);
	else if(flag == 1)
		bytes = 37;
	else if(flag == 2)
		bytes = 50;
	else
		bytes = 71;
	charbytes = myitoa(bytes);
	strcat(ans,charbytes);
	free(charbytes);
	strcat(ans,"\r\n");
	strcat(ans,"Content-Type: text/html\r\n");
	strcat(ans,"Connection: Closed\r\n\r\n");
	*curlen = strlen(ans);
	if(flag == 1){
		*curlen += 37;
		ans = (char *)realloc(ans,(*curlen)*sizeof(char));
		strcat(ans,"<html>Bad request received!</html>\r\n");
	}
	else if(flag == 2){
		*curlen += 52;
		ans = (char *)realloc(ans,(*curlen)*sizeof(char));
		strcat(ans,"<html>Sorry dude, couldn't find this file!</html>\r\n");
	}
	else if(flag == 3){
		*curlen += 73;
		ans = (char *)realloc(ans,(*curlen)*sizeof(char));
		strcat(ans,"<html>Trying to access this file but don't think I can make it!</html>\r\n");
	}
	else{

		*curlen += strlen(cppage) + 3;//for \0 and in the end \r\n
		ans = (char *)realloc(ans,(*curlen)*sizeof(char));
		strcat(ans,cppage);
		strcat(ans,"\r\n");
		free(cppage);
	}
	*pagebytes = bytes;
	return ans;
}


//function 5
void fillZeros(int **array,int w){
	for(int i=0;i<w;i++)
		(*array)[i]=0;
}

//function 6
int findChildPos(pid_t chid,pid_t *pids,int w){
	int pos = 0;
	for(int i=0;i<w;i++)
		if(pids[i]==chid){
			pos=i;
			break;
		}
	return pos;
}


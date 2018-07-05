#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>


//function 1
int guard(int ret,char * error,int exit_code){
	if(ret < 0){perror(error);exit(exit_code);}
	return ret;
}


//function 2
int read_command(int fd,char **buf,int size){
	char c;
	int n;
	int counter=0;
	while(1){
		if((n=read(fd,&c,1)) < 0) return n;
		if(!n){
			strcpy((*buf),"_BROKEN_FIFO_");
			counter = 13;
			break;
		}
		if(counter == size){//then realloc 
			size *=2;
			(*buf) = realloc((*buf),size*sizeof(char));
		}
		if(c == '\0' || c == EOF){
			(*buf)[counter] = '\0';
			break;
		}
		else (*buf)[counter++] = c;
	}
	return counter+1;
}


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
	char *ptr = malloc((c+1)*sizeof(char));
	ptr[c--] = '\0';
	if(value == 0) ptr[0] = '0';
	while(value){
		ptr[c--] = value%10 + '0';
		value/=10;
	}
	return ptr;
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


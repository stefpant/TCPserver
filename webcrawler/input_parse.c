#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include<dirent.h>

int isDirEmpty(char* gdir){
	int n=0;
	struct dirent *d;
	DIR *dir = opendir(gdir);
	if(dir == NULL)//gdir not a directory
		return -1;
	while((d = readdir(dir)) != NULL){
		if(++n > 2) {closedir(dir);return -2;}//not empty gdir
	}
	closedir(dir);
	return 0;//empty dir
}

//check arguments and for the given document as input initializes the map(DocContent)
int input_parse(int argc,char* argv[],int* s_port,int* c_port,int* n_threads,char** rootdir,
		char** hostid,char** s_url){
	int i;
	int flag1=0,flag2=0,flag3=0,flag4=0,flag5=0;
	for(i=1;i<argc;i++){
		if(strcmp(argv[i],"-d")==0){//then next argument must be the input directory
			if(++i==argc){//error if nothing next
				printf("Missing input file...\n");
				return -2;
			}
			flag1++;
			int len = strlen(argv[i]);
			char c = argv[i][len - 1];
			*rootdir = malloc((len+1)*sizeof(char));
			strcpy((*rootdir),argv[i]);
			if(c == '/'){
				(*rootdir)[len - 1] = '\0';
			}

			int err = isDirEmpty(*rootdir);
			if(err == -1){
				printf("Given directory does not exist!\n");
				return -2;
			}
			else if(err == -2){
				printf("Given directory not empty!\n");
				return -2;
			}
		}
		else if(strcmp(argv[i],"-t")==0){//if found this flag change the t
			if(i+1==argc){//if value for t missing just go on with t=5
				printf("Value for t not found.Continue by default with 5 threads\n");
				break;
			}
			*n_threads = atoi(argv[++i]);
			if(!(*n_threads)){//fails if k=0 or argument not a number
				printf("Try again with t>0\n");
				return -2;
			}
		}
		else if(strcmp(argv[i],"-p")==0){
			if(i+1==argc){
				printf("Missing serving port!\n");
				return -2;
			}
			flag2++;
			*s_port = atoi(argv[++i]);
			if(!(*s_port)){//fails if k=0 or argument not a number
				printf("Try again with serving_port>0\n");
				return -2;
			}
		}
		else if(strcmp(argv[i],"-c")==0){
			if(i+1==argc){
				printf("Missing command port!\n");
				return -2;
			}
			*c_port = atoi(argv[++i]);
			flag3++;
			if(!(*c_port)){//fails if k=0 or argument not a number
				printf("Try again with command_port>0\n");
				return -2;
			}
		}
		else if(strcmp(argv[i],"-h")==0){
			if(i+1==argc){
				printf("Missing host or id!\n");
				return -2;
			}
			flag4++;
			int len = strlen(argv[++i]);
			*hostid = malloc((len+1)*sizeof(char));
			strcpy((*hostid),argv[i]);
		}
		//else if(strcmp(argv[i],"-sorted")==0){
		//	*sorted = 1;//if found this flag sort the trie
		//}
		else{//here must be the starting url and be the last argument to succeed
			if(i+1 != argc){
				printf("%s\n",argv[i]);
				printf("Arguments may be wrong placed!\n");
				return -2;
			}
			flag5++;
			int len = strlen(argv[i]);
			*s_url = malloc((len+1)*sizeof(char));
			strcpy((*s_url),argv[i]);
		}
	}
	if(!flag1 || !flag2 || !flag3 || !flag4 || !flag5){
		printf("Missing some arguments!\n");
		return -2;
	}

	return 0;
}

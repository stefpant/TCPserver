#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>

//In success: return all read bytes
int read_doc(char *name,char ***map,int *lines){
	char c,pr_c;
	FILE* fp;
	*lines = 0;
	int counter = 0;//byte counter here
	if((fp=fopen(name,"r"))==NULL){
		printf("Given file doesn't exists!\n");
		return -1;
	}
	if(fp == NULL ){fclose(fp); return -1;}//error
	while((c=getc(fp)) != EOF){//1st parsing to find the lines
		if(c == '\n' && pr_c != '\n') (*lines)++;
		pr_c = c;
	}
	if(!(*lines)){printf("Empty input file!\n");fclose(fp);return -2;}
	//printf("Read %d folders!\n",*lines);
	*map = malloc((*lines)*sizeof(char*));
	if(fseek(fp,0L,SEEK_SET) !=0) {fclose(fp); return -2;}
	for(int i=0;i<(*lines);i++){
		fscanf(fp,"%m[^\n]s",&(*map)[i]);//read and save line
		getc(fp);//skip the remaining '\n'
		counter += strlen((*map)[i]) + 1;//the '+1' for '\n'
	}
	fclose(fp);
	return counter;
}


//check arguments and for the given document as input initializes the map(DocContent)
int input_parse(int argc,char* argv[],int* s_port,int* c_port,int* n_threads,char** rootdir){
	int i;
	int flag1=0,flag2=0,flag3=0;
	for(i=1;i<argc;i++){
		if(strcmp(argv[i],"-d")==0){//then next argument must be the input file
			if(++i==argc){//error if nothing next
				printf("Missing input file...\n");
				return -2;
			}
			//needs the '-i' argument to success else we haven't got the input file
			//and return failure
			flag1++;
			int len = strlen(argv[i]);
			char c = argv[i][len - 1];
			*rootdir = malloc((len+1)*sizeof(char));
			strcpy((*rootdir),argv[i]);
			if(c == '/'){
				(*rootdir)[len - 1] = '\0';
			}

			if(0 != access((*rootdir),F_OK)){
				if(ENOENT == errno){
					printf("Given directory does not exists!\n");
					return -2;
				}
			}
		}
		else if(strcmp(argv[i],"-t")==0){//if found this flag change the w
			if(i+1==argc){//if value for w missing just go on with w=3
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
		//else if(strcmp(argv[i],"-sorted")==0){
		//	*sorted = 1;//if found this flag sort the trie
		//}
		else{//for every other argument/flag or whatever
			printf("Unrecognized arguments!\n");
			return -2;//exits 
		}
	}
	if(!flag1 || !flag2 || !flag3){
		printf("Missing some arguments!\n");
		return -2;
	}

	return 0;
}

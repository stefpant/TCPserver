#include<stdio.h>
#include<stdlib.h>
#include<string.h>


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
int input_parse(int argc,char* argv[],char ***docmap,int* w,int* sorted,int *lines,char** fifoname,char** fifoname2){
	char *docfile = NULL;//just to be sure
	int i;
	int flag=0;
	for(i=1;i<argc;i++){
		if(strcmp(argv[i],"-d")==0){//then next argument must be the input file
			if(++i==argc){//error if nothing next
				printf("Missing input file...\n");
				return -2;
			}
			//needs the '-i' argument to success else we haven't got the input file
			//and return failure
			flag = 1;
			docfile = malloc(strlen(argv[i])+1);
			strcpy(docfile,argv[i]);
			//printf("docfile = %s\n",docfile);
		}
		else if(strcmp(argv[i],"-w")==0){//if found this flag change the w
			if(i+1==argc){//if value for w missing just go on with w=3
				printf("Value for k not found.Continue with default value(w=3)\n");
				break;
			}
			*w = atoi(argv[++i]);
			if(!(*w)){//fails if k=0 or argument not a number
				printf("Try again with w!=0\n");
				free(docfile);
				return -2;
			}
		}
		else if(strcmp(argv[i],"-sorted")==0){
			*sorted = 1;//if found this flag sort the trie
		}
		else if(strcmp(argv[i],"-n")==0){
			if(++i==argc){//error if nothing next
				printf("Missing 1st fifo name...\n");
				return -2;
			}
			int len = strlen(argv[i]);
			*fifoname = malloc((len+1)*sizeof(char));
			strcpy((*fifoname),argv[i]);
			if(++i==argc){//error if nothing next
				printf("Missing 2nd fifo name...\n");
				return -2;
			}
			len = strlen(argv[i]);
			*fifoname2 = malloc((len+1)*sizeof(char));
			strcpy((*fifoname2),argv[i]);
		}
		else{//for every other argument/flag or whatever
			printf("Unrecognized arguments!\n");
			free(docfile);
			return -2;//exits 
		}
	}
	if(!flag){
		printf("Input file not found!\n");
		return -2;
	}

	int error = read_doc(docfile,docmap,lines);
	free(docfile);
	if(error<0) return -2;
	return 0;
}

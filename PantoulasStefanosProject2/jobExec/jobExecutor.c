#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<errno.h>
#include<string.h>
#include<signal.h>

#include"functions.h"
#include"fifo_fcts.h"
#include"parent.h"
#include"child.h"
#include"../trie/input_parse.h"

int workers = 1;//global var-->used in parent

int main(int argc,char* argv[]){
	signal(SIGINT,SIG_IGN);
	signal(SIGTERM,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);//ignore signals before reaching the parent function
	int w = 3;//by default 3 workers
	int sorted = 0,lines = 0;
	char **docmap = NULL;
	char* fifoname=NULL;
	char* fifoname2=NULL;
	pid_t *processids = NULL;

	//signal(SIGINT,SIG_IGN);//ignore interupts ( ctr+C )

	//step 1: read arguments
	if(input_parse(argc,argv,&docmap,&w,&sorted,&lines,&fifoname,&fifoname2) < 0){
		printf("Parsing arguments error,exiting...\n");
		free(fifoname);
		free(fifoname2);
		exit(7);
	}

	if(lines < w){
		w = lines;
		//printf("Too much workers for given docfile.\n");
		//printf("Continue with %d workers!\n",w);
	}
	workers = w;
//	for(int i=0;i<lines;i++)
//		printf("line %d:%s\n",i+1,docmap[i]);

	//step 2: create and init structs for fifo infos
	fifo_info * parentfifos;//parent needs all fifos
	fifo_info * childfifos;//childs just 2 fifos to RD-WR from/to parent
	parentfifos = malloc(2*w*sizeof(fifo_info));
	childfifos = malloc(2*sizeof(fifo_info));

	initNames(&parentfifos,w);

	//step 3: make fifos
	for(int i=0;i<2*w;i++){
		//printf("%s\n",parentfifos[i].name);
		if(mkfifo(parentfifos[i].name,0666) == -1){
			if(errno != EEXIST){
				perror("mkfifo");
				exit(21);
			}
			else{//if fifo exists unlink it and make it again :-)
				unlink(parentfifos[i].name);
				if(mkfifo(parentfifos[i].name,0666) == -1){
					perror("mkfifo isn't working");
					exit(99);
				}
			}
		}
	}

	processids = malloc(w*sizeof(pid_t));
	pid_t pid;
	//step 4: fork childs and open fifos
	for(int i=0;i<w;i++){
		guard(pid = fork(),"fork failed",22);
		processids[i] = pid;//just to save pid in same var
		if(pid>0){//parent
			parentfifos[2*i].fd = open(parentfifos[2*i].name,O_WRONLY);
			parentfifos[2*i+1].fd = open(parentfifos[2*i+1].name,O_RDONLY);
		}
		else{//child
			for(int j=0;j<lines;j++) free(docmap[j]);free(docmap);
			childfifos[0].name = malloc(strlen(parentfifos[2*i].name) + 1);
			strcpy(childfifos[0].name,parentfifos[2*i].name);
			childfifos[0].fd = open(childfifos[0].name,O_RDONLY);
			childfifos[1].name = malloc(strlen(parentfifos[2*i+1].name) + 1);
			strcpy(childfifos[1].name,parentfifos[2*i+1].name);
			childfifos[1].fd = open(childfifos[1].name,O_WRONLY);
			for(int j=0;j<2*w;j++) free(parentfifos[j].name);
			free(parentfifos);//free parent's stuct
			free(fifoname);
			free(fifoname2);
			break ;//exit loop
		}
	}
	if(pid>0) free(childfifos);//for parent 
	else free(processids);//for childs
	//ready for IPC :-)

	if(pid > 0){
		int FIFOfd = open(fifoname,O_RDONLY);//open fifo to read from crawler
		int FIFOfd2 = open(fifoname2,O_WRONLY);//open fifo to write to crawler
		parentExec(parentfifos,docmap,w,lines,processids,sorted,FIFOfd,FIFOfd2);//main function
		for(int j=0;j<lines;j++) free(docmap[j]);free(docmap);
		free(processids);
		for(int i=0;i<2*w;i++)
			close(parentfifos[i].fd);
		close(FIFOfd);
		close(FIFOfd2);
	}
	else childExec(childfifos,sorted);//main function

	pid_t chret;
	int chres;
	while((chret = wait(&chres))>0){}
		//printf("Child %d search %d words!\n",(int)chret,WEXITSTATUS(chres));

	for(int i=0;i<2*w;i++){
		unlink(parentfifos[i].name);
		free(parentfifos[i].name);
	}
	free(parentfifos);
	printf("Job Executor about to exit!\n");
	return 0;
}

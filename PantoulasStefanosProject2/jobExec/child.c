#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<dirent.h>
#include<fcntl.h>
#include<string.h>
#include<math.h>
#include<time.h>
#include<signal.h>
#include<errno.h>

#include"functions.h"
#include"fifo_fcts.h"
#include"PCOptions.h"

#include"../trie/input_parse.h"
#include"../trie/trie.h"
#include"../trie/trieInsertion.h"

#define BUFSIZE 256

typedef struct filemap{
	char **map;
	char *name;
	int lines;
} filemap;

typedef struct trie_info{
	node* trieyo;
	int wc;//word counter
	int bc;//byte counter
	int lc;//line counter
} trie_info;

typedef struct query_info{
	char* word;
	node* last;//the node that has the posting list of word(NULL if word not found)
} query_info;

volatile sig_atomic_t flagCINT = 0;
volatile sig_atomic_t flagUSR1 = 0;

void Csighandler(int sig){
	if(sig == SIGINT || sig == SIGTERM || sig == SIGQUIT || sig == SIGPIPE){
		flagCINT=1;
	}
	else if(sig == SIGUSR1){
		flagUSR1 = 1;
	}
}

void mergeNodeInfo(int ***A,int *Ac,int **B,int Bc);

void childExec(fifo_info* cfifos, int sorted){
//	signal(SIGINT,SIG_IGN);
	struct sigaction sa;
	sa.sa_handler = Csighandler;
	sa.sa_flags = 0;
	sigfillset(&(sa.sa_mask));
	if(sigaction(SIGINT,&sa,NULL) < 0)
		perror("Can't handle SIGINT");
	if(sigaction(SIGQUIT,&sa,NULL) < 0)
		perror("Can't handle SIGQUIT");
	if(sigaction(SIGTERM,&sa,NULL) < 0)
		perror("Can't handle SIGTERM");
	if(sigaction(SIGUSR1,&sa,NULL) < 0)
		perror("Can't handle SIGUSR1");
	if(sigaction(SIGPIPE,&sa,NULL) < 0)
		perror("Can't handle SIGPIPE");
	char *buf = NULL;
	char *word = NULL;
	char *filepath = NULL;
	int flen;
	DIR* dirptr =NULL;
	struct dirent* direntp;

	time_t timer;
	char timebuffer[26];
	struct tm* tm_info;

	int searchwordcount = 0;//value to return when i exit

	char *log="../PantoulasStefanosProject2/jobExec/log/Worker_";//creating the logfile names for every child
	char *logbuf;
	logbuf = myitoa((int)getpid());//has in the end the '\0'
	char *logname = malloc((49 + itol((int)getpid()))*sizeof(char));
	strcpy(logname,log);
	strcat(logname,logbuf);
	free(logbuf);

	FILE* fp;//lets open the logfile now
	if((fp=fopen(logname,"w"))==NULL){
		perror("fopen");
		exit(49);
	}


	//fprintf(fp,"%d: %s\n",(int)getpid(),timebuffer);


	filemap *docmaps = malloc(sizeof(filemap));//k maps for every readed file
	//char ***docmaps = malloc(sizeof(char**));//
	//int *lines = malloc(sizeof(int));//lines for every map
	trie_info *mytrie = malloc(sizeof(trie_info));
	mytrie->trieyo=nodeConstruct('a');//random init the root
	mytrie->wc = 0;
	mytrie->bc = 0;
	mytrie->lc = 0;
	int mc = 0;//mapcounter
	int tempbc;//temp byte counter

	int n;
	int flag=0;
	int empty_process_flag = 0;

	while(1){//reading the k directories sent by parent 
		buf = malloc((BUFSIZE+1)*sizeof(char));
		guard(n = read_command(cfifos[0].fd,&buf,BUFSIZE),"child read error",12);
		if(!strcmp(buf,"_BROKEN_FIFO_")) {flagCINT = 1; flag = 1;}
		if((!strcmp(buf,"/stop") || n==0 )&& strcmp(buf,"_BROKEN_FIFO")) flag=1;
		else{//open and read directory :-)
			//printf("Im child:%d and read:%s\n",(int)getpid(),buf);
			if((dirptr = opendir(buf)) == NULL)
				printf("Cannot open directory:%s\n",buf);
			else{
				empty_process_flag = 1;//process not empty
				while((direntp = readdir(dirptr)) != NULL){
					if(direntp->d_type == DT_REG){//file to save
						//printf("Im child:%d and read:%s\n",(int)getpid(),direntp->d_name);
						docmaps = realloc(docmaps,(mc+1)*sizeof(filemap));
						//lines = realloc(lines,(mapcounter+1)*sizeof(int));
						flen = strlen(buf)+strlen(direntp->d_name)+2;
						filepath = malloc(flen*sizeof(char));
						strcpy(filepath,buf);
						strcat(filepath,"/");
						strcat(filepath,direntp->d_name);
						filepath[flen-1] = '\0';
						if((tempbc = read_doc(filepath,&(docmaps[mc].map),&(docmaps[mc].lines)))<0){
							printf("Error opening the file '%s'\n",filepath);
							free(filepath);
							continue;
						}
						mytrie->bc += tempbc;
						mytrie->lc += docmaps[mc].lines;
						//else printf("Child %d read %d lines!(%s)\n",(int)getpid(),docmaps[mapcounter].lines,direntp->d_name);
						docmaps[mc].name = filepath;
						mytrie->wc += createTrie(docmaps[mc].map,docmaps[mc].lines,docmaps[mc].name,mc,&(mytrie->trieyo),sorted);
						mc++;
					}
				}
				closedir(dirptr);
			}
		}
		free(buf);
		buf = NULL;
		//return ;
		if(flag) break;
	}

	if(write(cfifos[1].fd,"/ready\0",(size_t)7)!=7){//send msg to parent,child ready
		perror("child write error");}

	int start;
	while(1){//waiting for parent commands
		buf = malloc((BUFSIZE+1)*sizeof(char));
		if(read_command(cfifos[0].fd,&buf,BUFSIZE) < 0) flagCINT = 1;//fifo broke,something happened to parent
		if(flagCINT == 1) strcpy(buf,"bye");
		//check if parent killed, by checking if parent's id==1
		//then take that as INTERUPT and exit
		if(getppid() == 1) flagCINT = 1;
		if(!strcmp(buf,"_BROKEN_FIFO_")) flagCINT = 1;
		start = nextWord(buf,&word,0);
		if(!strcmp(word,"/exit") || flagCINT==1){//got exit command from parent or signal to terminate
			//printf("#au revoir\n");
			free(buf);
			free(word);
			break;
		}
		else if(!strcmp(word,"/wc")){
			//struct trie_info has all the needed infos :-)
			time(&timer);//save time in timebuffer
			tm_info = localtime(&timer);
			strftime(timebuffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
			free(word);
			word = NULL;
			if(empty_process_flag){//if trie not empty
				flen = itol(mytrie->bc)+itol(mytrie->wc)+itol(mytrie->lc)+3;
				word = malloc(flen*sizeof(char));
				char* tempbuf = myitoa(mytrie->bc);
				strcpy(word,tempbuf);
				strcat(word," ");
				free(tempbuf);
				tempbuf = myitoa(mytrie->wc);
				strcat(word,tempbuf);
				strcat(word," ");
				free(tempbuf);
				tempbuf = myitoa(mytrie->lc);
				strcat(word,tempbuf);
				strcat(word,"\0");
				free(tempbuf);
				if(write(cfifos[1].fd,word,flen)!=flen){//wc:results
					perror("child write error");}
				//free(word);
			}
			else{
				word = malloc(6*sizeof(char));
				strcpy(word,"0 0 0\0");
				if(write(cfifos[1].fd,"0 0 0\0",(size_t)6)!=6){//empty :'(
					perror("child write error");}
			}
			fprintf(fp,"%s : wc : [NULL] : %s\n",timebuffer,word);//write to log
			free(word);
		}
		else if(!strcmp(word,"/maxc") || !strcmp(word,"/minc")){
			int max = 1;//command = /maxc
			nc_pair *nc = NULL;
			if(!strcmp(word,"/minc")) max=0;//or /minc
			free(word);
			nextWord(buf,&word,start);
			//printf("read %s with max=%d\n",word,max);
			nc = searchWordInTrie(mytrie->trieyo,word,sorted,max);
			time(&timer);
			tm_info = localtime(&timer);
			strftime(timebuffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
			if(max)
				fprintf(fp,"%s : maxcount : %s : ",timebuffer,word);
			else
				fprintf(fp,"%s : mincount : %s : ",timebuffer,word);

			free(word);
			if(getNCname(nc) != NULL){//form to send:"PATH COUNTER"
				flen = strlen(getNCname(nc)) + itol(getNCcounter(nc)) + 2;
				word = malloc(flen*sizeof(char));
				char* tempbuf = myitoa(getNCcounter(nc));
				strcpy(word,getNCname(nc));
				strcat(word," ");
				strcat(word,tempbuf);
				strcat(word,"\0");
				free(tempbuf);
				if(write(cfifos[1].fd,word,flen)!=flen){//wc:results
					perror("child write error");}
				fprintf(fp,"%s\n",word);//append the result
				free(word);
			}
			else{//word not found trie,send "-"
				if(write(cfifos[1].fd,"-\0",(size_t)2)!=2){//empty :'(
					perror("child write error");}
				fprintf(fp,"-\n");
			}
			free(nc);
		}
		else if(!strcmp(word,"/search")){
			flagUSR1 = 0;
			//save words from search in array
			int maxsw = 8;
			query_info *qinfo = malloc(maxsw*sizeof(query_info));//start with 2^3 and increase..
			int swcounter = 0;
			while(start!=-1){//for every word save their info(word,node to PL )
				start = nextWord(buf,&(qinfo[swcounter].word),start);//save word
				swcounter++;
				if(swcounter == maxsw){
					maxsw*=2;
					qinfo = realloc(qinfo,maxsw*sizeof(query_info));
				}
			}

			searchwordcount += swcounter;

			qinfo = realloc(qinfo,swcounter*sizeof(query_info));//resize
			int **node_info = NULL;
			int **merged_node_info = NULL;
			int mnicounter = 0;//merged node info counter
			int nicounter = 0;//node info counter
			for(int i=0;i<swcounter;i++){
				if(flagUSR1 == 1) break;//stop merging results,deadline passed
				//find last node of every word
				qinfo[i].last = getWordLastNode(mytrie->trieyo,qinfo[i].word,sorted);
				//fprintf(fp,"Process %d read '%s'\n",(int)getpid(),qinfo[i].word);
				getNodeData(qinfo[i].last, &node_info, &nicounter);
				if(qinfo[i].last == NULL) continue;//word not found check next
				mergeNodeInfo(&merged_node_info,&mnicounter,node_info,nicounter);//merge results from words
				for(int u=0;u<nicounter;u++)
					free(node_info[u]);
				free(node_info);
				node_info = NULL;
			}
	//from merged_node_info get:
	//	1.file name with full path -->docmaps[merged_node_info[i][0]].name
	//	2.no of line --> merged_node_info[i][1]
	//	3.line's content --> docmaps[merged_node_info[i][0]].map[merged_node_info[i][1]]

			int wlen;
			char *tempbuf = NULL;
			char *strline = NULL;

			if(flagUSR1 == 0){//if flag not changed parent still waits for results
				for(int u=0;u<mnicounter;u++){
					wlen = strlen(docmaps[merged_node_info[u][0]].name) + itol(merged_node_info[u][1] + 1);
					wlen += strlen(docmaps[merged_node_info[u][0]].map[merged_node_info[u][1]]) + 3;//2 for spaces + '\0'
					tempbuf = malloc(wlen*sizeof(char));
					strcpy(tempbuf,docmaps[merged_node_info[u][0]].name);
					strcat(tempbuf," ");
					strline = myitoa(merged_node_info[u][1] + 1);
					strcat(tempbuf,strline);
					strcat(tempbuf," ");
					strcat(tempbuf,docmaps[merged_node_info[u][0]].map[merged_node_info[u][1]]);
					tempbuf[wlen-1] = '\0';//tempbuf ready to write in fifo
					if(write(cfifos[1].fd,tempbuf,wlen)!=wlen)
						perror("2child write error");
					free(strline);
					strline = NULL;
					free(tempbuf);
					tempbuf = NULL;
				}
				if(write(cfifos[1].fd,"_STOP_\0",7)!=7)
					perror("1child write error");
			}
			for(int u=0;u<mnicounter;u++)
				free(merged_node_info[u]);
			free(merged_node_info);

			for(int i=0;i<swcounter;i++){//write results in log
				char *spaths=NULL;
				time(&timer);
				tm_info = localtime(&timer);
				strftime(timebuffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
				fprintf(fp,"%s : search : %s",timebuffer,qinfo[i].word);
				spaths = getNamesAsString(qinfo[i].last);
				if(spaths==NULL) fprintf(fp,"\n");
				else {fprintf(fp,"%s\n",spaths);free(spaths);}
			}

			for(int i=0;i<swcounter;i++)free(qinfo[i].word);
			free(qinfo);
			free(word);
		}
		free(buf);
	}
	//free maps
	for(int i=0;i<mc;i++){
		for(int j=0;j<docmaps[i].lines;j++)
			free((docmaps[i]).map[j]);
		free((docmaps[i]).map);
		free((docmaps[i]).name);
	}
	free(docmaps);
	nodeDestruct(mytrie->trieyo);
	free(mytrie);
	free(logname);
	fclose(fp);

	if(write(cfifos[1].fd,"/exit_success\0",(size_t)14)!=14){//exit:success
		printf("Probably parent is dead :-(\n");}
	close(cfifos[0].fd);
	close(cfifos[1].fd);
	free(cfifos[0].name);
	free(cfifos[1].name);
	free(cfifos);
	exit(searchwordcount);
}



void mergeNodeInfo(int ***A,int *Ac,int **B,int Bc){
	if((*A) == NULL){//copy B array to A
		(*A) = malloc(Bc*sizeof(int*));
		for(int i=0;i<Bc;i++){
			(*A)[i] = malloc(2*sizeof(int));
			(*A)[i][0] = B[i][0];
			(*A)[i][1] = B[i][1];
		}
		*Ac = Bc;
		return;
	}
	else if(Bc == 0) return;
	int p1=0,p2=0;
	int fcounter=0;
	int **final = malloc((*Ac + Bc)*sizeof(int*));//allocate for max elements
	while(1){
		if(flagUSR1 == 1) break;//reached deadline stop
		if(p1==*Ac){
			while(p2!=Bc){
				final[fcounter] = malloc(2*sizeof(int));
				final[fcounter][0] = B[p2][0];
				final[fcounter][1] = B[p2][1];
				p2++;fcounter++;
			}
			break;
		}
		if(p2==Bc){
			while(p1!=*Ac){
				final[fcounter] = malloc(2*sizeof(int));
				final[fcounter][0] = (*A)[p1][0];
				final[fcounter][1] = (*A)[p1][1];
				p1++;fcounter++;
			}
			break;
		}
		final[fcounter] = malloc(2*sizeof(int));
		if((*A)[p1][0]<B[p2][0]){//insert in final sorted
			final[fcounter][0] = (*A)[p1][0];
			final[fcounter++][1] = (*A)[p1++][1];
		}
		else if((*A)[p1][0]>B[p2][0]){
			final[fcounter][0] = B[p2][0];
			final[fcounter++][1] = B[p2++][1];
		}
		else{//now lets check second member of array
			if((*A)[p1][1]<B[p2][1]){
				final[fcounter][0] = (*A)[p1][0];
				final[fcounter++][1] = (*A)[p1++][1];
			}
			else if((*A)[p1][1]>B[p2][1]){
				final[fcounter][0] = B[p2][0];
				final[fcounter++][1] = B[p2++][1];
			}
			else{
				final[fcounter][0] = (*A)[p1][0];
				final[fcounter++][1] = (*A)[p1++][1];
				p2++;
			}
		}
	}
	if(flagUSR1 == 1){//reached deadline free(final) and return
		for(int i=0;i<fcounter;i++)
			free(final[i]);
		free(final);
		return;//and in childs main check flag and stop!
	}
	if(!fcounter) return;
	for(int h=0;h<*Ac;h++) free((*A)[h]);
	if(*Ac!=0) free(*A);
	*A = realloc(final,fcounter*sizeof(int*));//in the end resize the array
	*Ac = fcounter;
	return;
}

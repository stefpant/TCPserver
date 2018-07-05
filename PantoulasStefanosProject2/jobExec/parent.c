#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<string.h>
#include<math.h>
#include<signal.h>
#include<time.h>
#include<poll.h>

#include"functions.h"
#include"fifo_fcts.h"
#include"PCOptions.h"
#include"child.h"
#include"../trie/trieInsertion.h"
#include"../trie/trie.h"

#define KNRM  "\x1B[0m"
#define KCYN  "\x1B[36m"

#define BUFSIZE 256

volatile sig_atomic_t flagINT = 0;
volatile sig_atomic_t deadChildCounter = 0;//used also as flag
sig_atomic_t* deadCHILD;//could die at same time(aka before handle) max w childs


void Psighandler(int sig, siginfo_t* info, void *secret){
	if(sig == SIGINT || sig == SIGTERM || sig == SIGQUIT){
		flagINT=1;
	}
	else if(sig == SIGCHLD){
		while(1){
			pid_t pid = waitpid(-1,NULL,WNOHANG);//wait for any child or ret 0
			if(pid <= 0) break;
			deadCHILD[deadChildCounter++] = pid;//save what you find
		}
	}
	else if(sig == SIGPIPE){}//ignore it
}

void parentExec(fifo_info *pfifos,char** map,int  w,int lines,pid_t *pids,int sorted,int CRfd,int CWfd){
	//signal(SIGINT,Psighandler);
	deadCHILD = malloc(w*sizeof(sig_atomic_t));
	struct sigaction sa;
	sa.sa_sigaction = Psighandler;
	sa.sa_flags = 0;
	sigfillset(&(sa.sa_mask));
	if(sigaction(SIGINT,&sa,NULL) < 0)
		perror("Can't handle SIGINT");
	if(sigaction(SIGQUIT,&sa,NULL) < 0)
		perror("Can't handle SIGQUIT");
	if(sigaction(SIGTERM,&sa,NULL) < 0)
		perror("Can't handle SIGTERM");
	if(sigaction(SIGCHLD,&sa,NULL) < 0)
		perror("Can't handle SIGCHLD");
	if(sigaction(SIGPIPE,&sa,NULL) < 0)
		perror("Can't handle SIGPIPE");

	int curlines = lines;
	int work,n;
	size_t length;
	char* command = NULL;
	char *buf = NULL;
	char *word = NULL;
	int Fbytes,Fwords,Flines;//for WC command
	//let's give my sons their homework

	int *dont_try_to_read = malloc(w*sizeof(int));
	fillZeros(&dont_try_to_read,w);

	//for(int i=0;i<w;i++) printf("Child:%d\n",(int)pids[i]);

	for(int i=0;i<w;i++){
		work = (int)ceil((double)curlines/(w-i));
		for(int j=0;j<work;j++){
			length = 1 + strlen(map[lines-curlines+j]);
			if(write(pfifos[2*i].fd,map[lines-curlines+j],length)!=length)//send command
				{/*perror("parent write error")*/dont_try_to_read[i] = 1;}//broken pipe,child is dead
			//printf("parent sent to %d the %s\n",i+1,map[lines-curlines+j]);
		}
		if(write(pfifos[2*i].fd,"/stop\0",(size_t)6)!=6)
			{/*perror("parent write error")*/dont_try_to_read[i] = 1;};//send command
		//printf("parent sent to %d the /stop\n",i+1);
		curlines -= work;
	}

	//all childs have now get their data and are ready for user commands

	int counter = 0;
	for(int i=0;i<w;i++){
		buf = malloc((BUFSIZE+1)*sizeof(char));
		if(read_command(pfifos[2*i+1].fd,&buf,BUFSIZE)<0){free(buf);continue;}
		//if(!strcmp(buf,"_BROKEN_FIFO_")) printf("Broken fifo found!\n");
		if(!strcmp(buf,"/ready")) counter++;
		free(buf);
	}

	if(counter == w) printf("JobExecutor just initialized!\n");

	int start;
	while(1){//waiting user commands
		fillZeros(&dont_try_to_read,w);
//*****************************HERE FORKS NEW CHILDS****************************
		while(deadChildCounter>0){//oooh my child(s) died let's fork some new
			pid_t chid = deadCHILD[--deadChildCounter];
			int chpos = findChildPos(chid,pids,w);
			//fork new child in old one's pos
			//close and unlink fifos,then create them and open again
			close(pfifos[2*chpos].fd);
			close(pfifos[2*chpos+1].fd);
			unlink(pfifos[2*chpos].name);
			unlink(pfifos[2*chpos+1].name);
			if(mkfifo(pfifos[2*chpos].name,0666) == -1){perror("mkfifo");exit(37);}
			if(mkfifo(pfifos[2*chpos+1].name,0666) == -1){perror("mkfifo");exit(37);}
			guard(pids[chpos] = fork(),"fork failed",22);
			if(pids[chpos]==0){
				//first let's free useless data
				free(dont_try_to_read);
				free(pids);
				for(int j=0;j<lines;j++) free(map[j]);free(map);
				//open again fifos to communicate with parent
				fifo_info * childfifos = malloc(2*sizeof(fifo_info));
				childfifos[0].name = malloc(strlen(pfifos[2*chpos].name) + 1);
				strcpy(childfifos[0].name,pfifos[2*chpos].name);
				childfifos[0].fd = open(childfifos[0].name,O_RDONLY);
				childfifos[1].name = malloc(strlen(pfifos[2*chpos+1].name) + 1);
				strcpy(childfifos[1].name,pfifos[2*chpos+1].name);
				childfifos[1].fd = open(childfifos[1].name,O_WRONLY);
				for(int j=0;j<2*w;j++) free(pfifos[j].name);
				free(pfifos);
				free(deadCHILD);
				childExec(childfifos,sorted);//main function for child
			}
			else{//now lets find and sent his direntories(plus open again fifos)
				pfifos[2*chpos].fd = open(pfifos[2*chpos].name,O_WRONLY);
				pfifos[2*chpos+1].fd = open(pfifos[2*chpos+1].name,O_RDONLY);
				curlines = lines;
				for(int i=0;i<chpos;i++){//skip some paths
					work = (int)ceil((double)curlines/(w-i));
					curlines-=work;
				}
				work = (int)ceil((double)curlines/(w-chpos));//our dicts are here
				sleep(1);

				for(int j=0;j<work;j++){
					length = 1 + strlen(map[lines-curlines+j]);
					if(write(pfifos[2*chpos].fd,map[lines-curlines+j],length)!=length)//send command
						{dont_try_to_read[chpos] = 1;}//broken pipe,child is dead
				}
				if(write(pfifos[2*chpos].fd,"/stop\0",(size_t)6)!=6)
					{dont_try_to_read[chpos] = 1;};//send command

				//child has now get their data and is ready for user commands
				buf = malloc((BUFSIZE+1)*sizeof(char));
				guard(n = read_command(pfifos[2*chpos+1].fd,&buf,BUFSIZE),"child read error",12);
				//if(!strcmp(buf,"_BROKEN_FIFO_")) printf("Broken fifo found!\n");
				if(!strcmp(buf,"/ready")) counter++;
				free(buf);
				printf("New child %d is ready!\n",pids[chpos]);
			}
		}
//******************************************************************************
//if any child get SIGKILL after that point
//we continue with the remaining childs
//to run the command and print results and
//in the next loop we fork and init new one(s)
		if(!flagINT){
			command = malloc(11*sizeof(char));//new
			//printf(KCYN "$>" KNRM);//colorized
			//scanf(" %m[^\n]s",&command);//read until '\n'
			guard(n = read_command(CRfd,&command,10),"crawler read error",12);
			//printf(":%s\n",command);
			if(deadChildCounter){//
				command = malloc(14*sizeof(char));
				strcpy(command,"CREATE_CHILD_");//got term signal
			}
			start = nextWord(command,&word,0);//get command[0]
		}
		else{
			start = 0;
			word = malloc(2*sizeof(char));
			strcpy(word,"-");
		}
		if(!strcmp(word,"/exit") || !strcmp(word,"\\exit") || flagINT==1){//COMMAND 1: EXIT or BAD SIG received
			if(start != -1 && !flagINT) printf("No parameters needed for EXIT command\n");
			if(flagINT)
				printf("Recieved termination signal,exiting...\n");

			signal(SIGCHLD,SIG_DFL);
			for(int i=0;i<w;i++){//send command to all childs
				if(write(pfifos[2*i].fd,"/exit\0",(size_t)6) != 6){
					/*perror("parent write error")*///some child is dead
					dont_try_to_read[i] = 1;//dont read from i process
				}
			}
			if(word != NULL)free(word);
			if(command != NULL)free(command);
			break;
		}
		else if(!strcmp(word,"/wc") || !strcmp(word,"\\wc")){//COMMAND 2: WC
			if(start != -1) printf("No parameters needed for EXIT command\n");
			for(int i=0;i<w;i++){//send command to all childs
				if(write(pfifos[2*i].fd,"/wc\0",(size_t)4)!=4){
					/*perror("parent write error")*/
					dont_try_to_read[i] = 1;//dont read from i process
				}
			}
			//another loop here to wait for results!
			Fbytes = 0;
			Fwords = 0;
			Flines = 0;

			buf = malloc((BUFSIZE+1)*sizeof(char));
			for(int i=0;i<w;i++){
				if(!dont_try_to_read[i]){
					guard(n = read_command(pfifos[2*i+1].fd,&buf,BUFSIZE),"child read error",12);
					if(strcmp(buf,"_BROKEN_FIFO_") && strcmp(buf,"/exit_success"))//check if child died between above write and read
						decode_plus(buf,&Fbytes,&Fwords,&Flines);//else got expected result
				}
			}
			free(buf);
			printf("Bytes=%d || Words=%d || Lines=%d\n",Fbytes,Fwords,Flines);
		}
		else if(!strcmp(word,"/maxcount") || !strcmp(word,"\\maxcount")){//COMMAND 3: MAXCOUNT
			free(word);
			start = nextWord(command,&word,start);
			if(start != -1){//check command's parameter
				printf("Wrong command syntax!\n");
				if(start != -2) free(word);
				free(command);
				continue;
			}
			length = 7 + strlen(word);
			buf = malloc(length*sizeof(char));
			strcpy(buf,"/maxc ");
			strcat(buf,word);//command ready to send
//			free(word);
			for(int i=0;i<w;i++){//send command to all childs
				if(write(pfifos[2*i].fd,buf,length)!=length){
					/*perror("parent write error")*/
					dont_try_to_read[i] = 1;
				}
			}
			free(buf);
			buf = NULL;
			int maxcounter = -1;
			char *myname=NULL;
			for(int i=0;i<w;i++){
				buf = malloc((BUFSIZE+1)*sizeof(char));
				guard(n = read_command(pfifos[2*i+1].fd,&buf,BUFSIZE),"child read error",12);
				if(strcmp(buf,"_BROKEN_FIFO_") && strcmp(buf,"/exit_success"))
					decode_MAX(buf,&myname,&maxcounter);
				free(buf);
			}
			if(myname == NULL)
				printf("Word '%s' doesn't exists in any document!\n",word);
			else{
				printf("Word: %s\nFile:%s\nMax:%d\n",word,myname,maxcounter);
				free(myname);
			}
		}
		else if(!strcmp(word,"/mincount") || !strcmp(word,"\\mincount")){//COMMAND 4: MINCOUNT
			free(word);
			start = nextWord(command,&word,start);
			if(start != -1){//check command's parameter
				printf("Wrong command syntax!\n");
				if(start != -2) free(word);
				free(command);
				continue;
			}
			length = 7 + strlen(word);
			buf = malloc(length*sizeof(char));
			strcpy(buf,"/minc ");
			strcat(buf,word);//command ready to send
//			free(word);
			for(int i=0;i<w;i++){//send command to all childs
				if(write(pfifos[2*i].fd,buf,length)!=length){
					/*perror("parent write error")*/
					dont_try_to_read[i] = 1;
				}
			}
			free(buf);
			buf = NULL;
			int mincounter = -1;
			char *myname=NULL;
			for(int i=0;i<w;i++){
				buf = malloc((BUFSIZE+1)*sizeof(char));
				guard(n = read_command(pfifos[2*i+1].fd,&buf,BUFSIZE),"child read error",12);
				if(strcmp(buf,"_BROKEN_FIFO_") && strcmp(buf,"/exit_success"))
					decode_MIN(buf,&myname,&mincounter);
				free(buf);
			}
			if(myname == NULL)
				printf("Word '%s' doesn't exists in any document!\n",word);
			else{
				printf("Word: %s\nFile:%s\nMin:%d\n",word,myname,mincounter);
				free(myname);
			}
		}
//		else if(!strcmp(word,"/search") || !strcmp(word,"\\search")){//COMMAND 5: SEARCH
		else if(!strcmp(word,"SEARCH")){//search command for crawler here
			int maxsw = 8;
			char **s_words = malloc(maxsw*sizeof(char *));//start with 2^3 and increase..
			int swcounter = 0;
			while(start!=-1){//for every word save their info(word,node to PL )
				start = nextWord(command,&(s_words[swcounter]),start);//save word
				swcounter++;
				if(swcounter == maxsw){
					maxsw*=2;
					s_words = realloc(s_words,maxsw*sizeof(char *));
				}
			}
			s_words = realloc(s_words,swcounter*sizeof(char *));//resize

			if(swcounter < 2){//then deadline is missing
				printf("Deadline is missing.Try again!\n");
				for(int i=0;i<swcounter;i++) free(s_words[i]);free(s_words);
				free(word); free(command);
				continue;
			}
			else if(!strcmp(s_words[swcounter-1],"-d")){//if not found -d
				printf("Deadline flag ('-d') is empty.Try again!\n");
				for(int i=0;i<swcounter;i++) free(s_words[i]);free(s_words);
				free(word); free(command);
				continue;
			}
			else if(strcmp(s_words[swcounter-2],"-d")){//if not found -d
				printf("Deadline flag ('-d') not found.Try again!\n");
				for(int i=0;i<swcounter;i++) free(s_words[i]);free(s_words);
				free(word); free(command);
				continue;
			}
			float deadline = atof(s_words[swcounter-1]);
			if(deadline == 0.0 && s_words[swcounter-1][0] != '0'){//check
				printf("Deadline should be a real number.Try again!\n");
				for(int i=0;i<swcounter;i++) free(s_words[i]);free(s_words);
				free(word); free(command);
				continue;
			}

			char *childcommand=NULL;
			int ccomcounter=7;// count the '/search'
			for(int i=0;i<swcounter-2 && i<10;i++)
				ccomcounter+=strlen(s_words[i]) + 1;//plus one for space

			childcommand = malloc((ccomcounter+1)*sizeof(char));

			strcpy(childcommand,"/search");
			for(int i=0;i<swcounter-2 && i<10;i++){//send max 10 words as query
				strcat(childcommand," ");
				strcat(childcommand,s_words[i]);
			}
			childcommand[ccomcounter] = '\0';

			if(swcounter == 2){
				printf("Empty search query.\n");
				free(word); free(command);
				free(childcommand);
				continue;
			}
			//free s words we checked and kept the deadline
			for(int i=0;i<swcounter;i++) free(s_words[i]);free(s_words);
			//now sent command to childs
			ccomcounter++;
			for(int i=0;i<w;i++){//send command to all childs
				if(write(pfifos[2*i].fd,childcommand,ccomcounter)!=ccomcounter){
					/*perror("parent write error")*/
					dont_try_to_read[i] = 1;
				}
			}
			clock_t start = clock();
			clock_t end;

			int lcounter = 0;//childs that answered
			int rdcounter = 0;//answered or dead childs
			float eltime;
			int ready;
			struct pollfd fds[w];
			for(int i=0;i<w;i++){
				fds[i].fd = pfifos[2*i+1].fd;
				fds[i].events = POLLIN;
			}
			while(1){
				end = clock();
				eltime = (float)(end-start)/CLOCKS_PER_SEC;
				if(rdcounter>=w || eltime>=deadline) break;

				int pollmsecs = 1000*(deadline-eltime);
				ready = poll(fds,w,pollmsecs);
				if(ready == 0) break;//deadline passed-some childs didn't answered
				for(int j=0;j<w;j++){
					if(dont_try_to_read[j] == 0){
						if(fds[j].revents & POLLIN){
							lcounter++;
							rdcounter++;
							if(!dont_try_to_read[j]){
								while(1){//read until get _STOP_ msg
									buf = malloc((BUFSIZE+1)*sizeof(char));
									if(read_command(fds[j].fd,&buf,BUFSIZE)<0){free(buf); break;}
									if(!strcmp(buf,"_STOP_")){free(buf); break;}
									if(!strcmp(buf,"_BROKEN_FIFO_")){free(buf); break;}//broken fifo found
									//printf("%s\n",buf);//just print what u get from childs
									write(CWfd,buf,strlen(buf)+1);//send answer to crawler
									free(buf);
								}
							}
						}
						else if((fds[j].revents & POLLHUP) || (fds[j].revents & POLLERR)){
							rdcounter++;
							dont_try_to_read[j] = 1;
						}
					}
				}
			}

			ready = poll(fds,w,100);//get the remaining results from childs if they have answered
			for(int j=0;j<w;j++){
				if(fds[j].revents & POLLIN){
					lcounter++;
					if(!dont_try_to_read[j]){
						while(1){//read until get _STOP_ msg
							buf = malloc((BUFSIZE+1)*sizeof(char));
							if(read_command(fds[j].fd,&buf,BUFSIZE)<0){free(buf); break;}
							if(!strcmp(buf,"_STOP_")){free(buf); break;}
							if(!strcmp(buf,"_BROKEN_FIFO_")){free(buf); break;}//broken fifo found
							//printf("%s\n",buf);//just print what u get from childs
							write(CWfd,buf,strlen(buf)+1);//send answer to crawler
							free(buf);
						}
					}
				}
			}

			if(lcounter<w)//then send signal to childs to not send their results
				for(int i=0;i<w;i++)//send SIGUSR1 to all childs
					kill(pids[i],SIGUSR1);//return -1 if pids[i] is 'dead' but we don't care here

			write(CWfd,"_STOP_\n\0",8);//send 'stop' to crawler
			//printf("%d out of %d workers answered!\n",lcounter,w);

			free(childcommand);
		}
		else if(!strcmp(word,"CREATE_CHILD_")){}
		else printf("Unknown command.Try again!\n");
		free(word);
		word = NULL;
		free(command);
		command = NULL;
	}

	counter = 0;
	for(int i=0;i<w;i++){//again wait for all childs to send:exit_success msg
		//if(!dont_try_to_read[i]){
			buf = malloc((BUFSIZE+1)*sizeof(char));
			guard(n = read_command(pfifos[2*i+1].fd,&buf,BUFSIZE),"child read error",12);
			if(!strcmp(buf,"/exit_success")) counter++;
			//if(!strcmp(buf,"_BROKEN_FIFO_")) printf("Broken fifo found!\n");
			free(buf);
		//}
	}

	//if(counter == w) printf("All child processes exited successfully!\n");

	free(deadCHILD);
	free(dont_try_to_read);
	return ;
}




#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<signal.h>
#include<pthread.h>
#include<stdbool.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<time.h>
#include<poll.h>
#include<fcntl.h>
#include<netdb.h>

#include"functions.h"
#include"input_parse.h"

#define MAXR 10 //pool size(resize every time(x2) when it reach the max)
#define BUFFERSIZE 128 //buffer size to read answer from server

pthread_mutex_t mtx;
pthread_mutex_t mtxcounter;
pthread_mutex_t mtxbytes;
pthread_mutex_t mtxwt;
pthread_cond_t cv_notempty;
pthread_cond_t cv_notfull;
int totalrc = 0;//for command STATS
int totalBytes = 0;//for command STATS
int waitingThreads=0;//to check if crawling is in progress
typedef struct {
	char** urls;//array of urls
	int size;
	int count;
	//int start;
	int end;
} urlpool;//pool of urls

pthread_mutex_t pumtx;
char** placedurls=NULL;
int pucounter=0;


int checkinPU(char** url){
	int counter=0;
	int flag=0;
	while(1){
		pthread_mutex_lock(&pumtx);
		if(counter==pucounter) break;//url not found
		if(!strcmp(placedurls[counter++],*url)) flag=1;
		pthread_mutex_unlock(&pumtx);
		if(flag) return 1;//found
	}
	//still mutex locked
	pucounter++;
	placedurls = realloc(placedurls,pucounter*sizeof(char*));
	placedurls[pucounter-1]=*url;
	pthread_mutex_unlock(&pumtx);
	return 0;
}

void destroyPU(void){
	//no need to lock just free memory
	for(int i=0;i<pucounter;i++)
		free(placedurls[i]);
	free(placedurls);
}

urlpool *up;

void urlpool_init(urlpool** up){
	(*up) = malloc(sizeof(urlpool));
	(*up)->urls = malloc(MAXR*sizeof(char*));
	(*up)->size = MAXR;
	//(*up)->start = 0;2
	(*up)->end = -1;
	(*up)->count = 0;
}

void urlpool_clear(urlpool** up){//clearing pool
	while(1){
		pthread_mutex_lock(&mtx);
		if( (*up)->count == 0 ){
			pthread_mutex_unlock(&mtx);
			return;
		}
		free( (*up)->urls[(*up)->end] );
		(*up)->end--;
		(*up)->count--;
		pthread_mutex_unlock(&mtx);
	}
	return;
}
void urlpool_destroy(urlpool** up){
	free((*up)->urls);
	free((*up));
}

void place(urlpool** up,char* url){// place url in buffer
	pthread_mutex_lock(&mtx);
	//while((*up)->count>=MAXR){3
	if((*up)->count>=(*up)->size){//full pool->realloc
		(*up)->size*=2;
		(*up)->urls=realloc((*up)->urls,(*up)->size*sizeof(char*));
		//printf("Pool found full\n");
		//pthread_cond_wait(&cv_notfull,&mtx);4
	}
	(*up)->end = ((*up)->end+1);//%MAXR;
	(*up)->urls[(*up)->end] = malloc((strlen(url)+1)*sizeof(char));
	strcpy((*up)->urls[(*up)->end],url);
	//printf("placed: %d\n",(*ra)->fds[(*ra)->end]);
	(*up)->count++;
	pthread_mutex_unlock(&mtx);
}

char* obtain(urlpool** up){//get request from buffer
	char* data;
	pthread_mutex_lock(&mtx);
	while( (*up)->count <= 0 ){
		//printf("Pool found empty\n");
		pthread_cond_wait(&cv_notempty,&mtx);
	}
	//data = malloc((strlen((*up)->urls[(*up)->start]) + 1)*sizeof(char));
	data = malloc((strlen((*up)->urls[(*up)->end]) + 1)*sizeof(char));
	//strcpy(data,(*up)->urls[(*up)->start]);
	strcpy(data,(*up)->urls[(*up)->end]);
	//free( (*up)->urls[(*up)->start] );
	free( (*up)->urls[(*up)->end] );
	//(*up)->start = ( (*up)->start + 1 ) % MAXR;
	(*up)->end--;
	(*up)->count--;
	pthread_mutex_unlock(&mtx);
	return data;
}



typedef struct{//useful data for every thread
	int port;
	char *hostname;
	char *savedir;
} tdata;



void *threadExec(void * data){
	tdata *mydata = (tdata *)data;
	char *url=NULL;
	char *url_page=NULL;
	char *req=NULL;
	char *buf=NULL;
	char* prt=myitoa(mydata->port);
	char *sturl=malloc((strlen(prt)+strlen(mydata->hostname)+9)*sizeof(char));
	strcpy(sturl,"http://");
	strcat(sturl,mydata->hostname);
	strcat(sturl,":");
	strcat(sturl,prt);
	free(prt);
	while( 1 ){//try to obtain a url and exit if u obtain "exit"
//~~~~~~~~~~~~~~~~thread trying to obtain url~~~~~~~~~~~~~~~~~~~~
		pthread_mutex_lock(&mtxwt);//maybe wait in pool
		waitingThreads++;
		pthread_mutex_unlock(&mtxwt);
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		url = obtain(&up);
		pthread_cond_signal(&cv_notfull);
//~~~~~~~~~~~~~~let parent know i got the url~~~~~~~~~~~~~~~~~~~~
		pthread_mutex_lock(&mtxwt);
		waitingThreads--;
		pthread_mutex_unlock(&mtxwt);
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if(!strcmp(url,"exit")) break;

		pthread_mutex_lock(&mtxcounter);
		totalrc++;
		pthread_mutex_unlock(&mtxcounter);

		//printf("%ld:received %s\n",pthread_self(),url);
		if( analyze_url(url,mydata->hostname,mydata->port,&url_page) < 0){
			printf("Wrong url found!\n");
			free(url_page);
			free(url);
			url_page=NULL;
			url = NULL;
			continue;
		}
//!!!never saving in pool urls from pages that already exists in savedir!!!

		req = create_request(url_page,mydata->hostname,mydata->port);
		//printf("%s\n",req);
//~~~~~~~~~~~create socket,connect and send request~~~~~~~~~~~
		int sock;
		struct sockaddr_in server;
		struct sockaddr * serverptr = (struct sockaddr *)&server;
		struct hostent *rem;
		if( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
			perror("socket");
			exit(9);
		}

		if(isIP(mydata->hostname)){
			struct in_addr myaddress;
			inet_aton(mydata->hostname,&myaddress);
			rem=gethostbyaddr((const char*)&myaddress,sizeof(myaddress),AF_INET);
			if(rem == NULL){perror("gethostbyaddr");exit(9);}
		}
		else{		
			if( (rem = gethostbyname(mydata->hostname) ) == NULL ) //socket for service port
				{perror("gethostbyname");exit(9);}
		}
		server.sin_family = AF_INET;
		memcpy(&server.sin_addr,rem->h_addr,rem->h_length);
		server.sin_port = htons(mydata->port);

		if( (connect(sock,serverptr,sizeof(server))) <0){
			if(errno != EINPROGRESS){//need to check this for non blocking connection
				perror("connect");
				exit(9);
			}
		}

		write(sock,req,strlen(req)+1);

		while( (buf = read_request(sock,BUFFERSIZE)) == NULL ){}
		//printf("%s\n",buf);

		//save page in savedir(content of page in buf)
		int bufp=save_page(url_page,mydata->savedir,buf);
		if(bufp<0) printf("Can't find page!\n");
		else{
			pthread_mutex_lock(&mtxbytes);
			totalBytes += (strlen(buf) - bufp);//page bytes:requestBytes-headerBytes
			pthread_mutex_unlock(&mtxbytes);
			char **newpages=NULL;
			int newpagescounter=0;
			newpages = search_new_pages(buf,bufp,&newpagescounter);
			//char *testpage=NULL;
			for(int i=0;i<newpagescounter;i++){
				//testpage=malloc((strlen(mydata->savedir)+strlen(newpages[i])+1)*sizeof(char));
				//strcpy(testpage,mydata->savedir);
				//strcat(testpage,newpages[i]);
				//if(0 != access(testpage,F_OK)){//page doesnot exist in savedir,place in pool
				char* newurl=malloc((strlen(sturl)+strlen(newpages[i])+1)*sizeof(char));
				strcpy(newurl,sturl);
				strcat(newurl,newpages[i]);
				if( checkinPU(&newurl)==0 ){
					place(&up, newurl);
					printf("Placing in pool:%s\n",newurl);
					pthread_cond_signal(&cv_notempty);
				}
				else free(newurl);//free only if url exists else save it in placedurls
				//}
				//free(testpage);
				free(newpages[i]);
			}
			free(newpages);
		}

		close(sock);
		free(url_page);
		free(url);
		free(req);
		free(buf);
		url_page=NULL;
		url = NULL;
		req = NULL;
		buf = NULL;
	}
	free(sturl);
	free(url);
	printf("thread exiting...\n");
	return NULL;
}


int main(int argc,char* argv[]){
	int s_port,c_port;
	int n_threads = 5;//by default 5 threads
	char *savedir = NULL;
	char *hostid = NULL;
	pid_t pid;//for calling jobExec and in the end wait termination
	char *starting_url = NULL;
	int flagSHUTDOWN=0;//when set 1 exit
	int flagSEARCH=1;//first time(if crawler ready) call jobExecutor
	int FIFOfd,FIFOfd2;
	printf("Parent %ld: just started!\n", pthread_self());
//~~~~~~~~~~~~~~~~~~~~~~~~~~PARSE ARGUMENTS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if(input_parse(argc,argv,&s_port,&c_port,&n_threads,&savedir,&hostid,&starting_url) < 0){
		free(savedir);
		free(hostid);
		free(starting_url);
		exit(9);
	}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~INIT: pool,mutexes,conditions~~~~~~~~~~~~~~~~~~~~~~~
	urlpool_init(&up);

	pthread_mutex_init(&mtx, 0);
	pthread_mutex_init(&mtxwt, 0);
	pthread_mutex_init(&pumtx, 0);
	pthread_mutex_init(&mtxcounter, 0);
	pthread_mutex_init(&mtxbytes, 0);
	pthread_cond_init(&cv_notempty, 0);
	pthread_cond_init(&cv_notfull, 0);
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~MAKING THREADS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	pthread_t *tids = NULL;
	tids = malloc(n_threads*sizeof(pthread_t));

	tdata *dat;//struct to send to threads
	dat = malloc(sizeof(tdata));
	dat->port = s_port;
	dat->savedir = malloc((strlen(savedir)+1)*sizeof(char));
	strcpy(dat->savedir,savedir);
	dat->hostname = malloc((strlen(hostid)+1)*sizeof(char));
	strcpy(dat->hostname,hostid);

	for(int i=0; i<n_threads; i++)//create threads to execute threadExec
		pthread_create(tids+i,NULL,threadExec, (void *)dat);

	printf("Parent %ld: just created new threads!\n", pthread_self());
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	pthread_mutex_lock(&pumtx);
	placedurls=malloc(sizeof(char*));
	pucounter++;
	placedurls[0]=starting_url;
	pthread_mutex_unlock(&pumtx);
	//place starting url in pool
	place(&up, starting_url);
	pthread_cond_signal(&cv_notempty);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	time_t start,localend;
	int hh,mm,ss;
	time(&start);//crawler started
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~MAKING SOCKETS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	int c_sock,newcsock;
	struct sockaddr_in server;
	struct sockaddr * serverptr = (struct sockaddr *)&server;
	if( (c_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) //socket for command port
		{perror("socket");exit(9);}
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(c_port);

	if( setsockopt(c_sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 },sizeof(int)) < 0 )
		{perror("setsockopt");exit(9);}
	if( bind(c_sock,serverptr,sizeof(server)) < 0 ){perror("bind");exit(9);}
	if( listen(c_sock,100) < 0 ){perror("listen");exit(9);}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~INIT POLL~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	struct pollfd pfd[1];
	pfd[0].fd = c_sock;
	pfd[0].events = POLLIN;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~LOOP:ACCEPTING CONNECTIONS~~~~~~~~~~~~~~~~~~~~~~~
	while( 1 ){//only on command port

		poll(pfd,1,-1);//wait a connection
		if(pfd[0].revents & POLLIN){//client on command port
			if( (newcsock = accept(c_sock,NULL,NULL)) < 0)
				{perror("accept");exit(9);}
			while(1){//1
				char *buf = malloc(9*sizeof(char));//should not be resized
				read_command(newcsock,&buf,9);
				if( !strcmp(buf,"STATS") ){
					time(&localend);
					localend -= start;
					hh = localend/3600;//creating time format
					localend -= hh*3600;
					mm = localend/60;
					ss = localend - mm*60;
					char* statrep=NULL;
					if(asprintf(&statrep,"Crawler up for %02d:%02d:%02d, downloaded %d pages, %d bytes\n",hh,mm,ss,totalrc,totalBytes) != -1){
						int len=strlen(statrep);
						write(newcsock,statrep,len+1);
						free(statrep);
					}
					//printf("Crawler up for %02d:%02d:%02d, downloaded %d pages, %d bytes\n",hh,mm,ss,totalrc,totalBytes);
				}
				else if( !strcmp(buf,"SHUTDOWN") ){
					close(newcsock);
					free(buf);
					flagSHUTDOWN=1;//2
					break;
				}
				else if( !strcmp(buf,"STOP") ){//3
					close(newcsock);//4
					free(buf);//5
					break;//6
				}//7
				else{
					char* ms=NULL;//maybe search command
					//printf("%s\n",buf);
					nextWord(buf,&ms,0);
					if( !strcmp(ms,"SEARCH") ){
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~check if crawling is over~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
						if( waitingThreads!=n_threads || up->count!=0 ){//pool not empty or some threads not waiting
							//printf("Crawling in progress,wait!\n");
							write(newcsock,"Crawling in progress,wait!\n",28);
							free(ms);free(buf);
							continue;
						}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
						
						if(flagSEARCH){//first time got SEARCH command,init some structs
							char* sdoc="docfile.txt";
							createDocfile(savedir,sdoc);
//~~~~~~~~~~~~~~~~~~~~fifo for crawler-jobExecutor communication~~~~~~~~~~~~~~~~~~~~~~
							if(mkfifo("../JECfifo",0666) == -1){
								if(errno != EEXIST){
									perror("mkfifo");
									exit(21);
								}
								else{//if fifo exists unlink it and make it again :-)
									unlink("../JECfifo");
									if(mkfifo("../JECfifo",0666) == -1){
										perror("mkfifo isn't working");
										exit(99);
									}
								}
							}
							if(mkfifo("../CJEfifo",0666) == -1){
								if(errno != EEXIST){
									perror("mkfifo");
									exit(21);
								}
								else{//if fifo exists unlink it and make it again :-)
									unlink("../CJEfifo");
									if(mkfifo("../CJEfifo",0666) == -1){
										perror("mkfifo isn't working");
										exit(99);
									}
								}
							}
							pid = fork();
							if(pid < 0){perror("fork");exit(87);}
							else if(pid == 0){//child call jobExecutor...
								close(newcsock);
								char *const args[]={//arguments for jobExecutor
												"../PantoulasStefanosProject2/jobExecutor",//executable
												"-d",//flag for docfile
												"docfile.txt",//docfile
												"-w",//flag for workers
												"5",//workers
												"-n",//flag for fifo names for C-jE communication
												"../JECfifo",//jE reads from C in this fifo
												"../CJEfifo",//C reads from jE in this fifo
												NULL};
								execv("../PantoulasStefanosProject2/jobExecutor",args);
								exit(54);//unexpected return here
							}
							//parent continue from here
							FIFOfd = open("../JECfifo",O_WRONLY);
							FIFOfd2 = open("../CJEfifo",O_RDONLY);
							flagSEARCH=0;//never again here
						}
						char* msg=malloc((strlen(buf)+9)*sizeof(char));
						strcpy(msg,buf);
						strcat(msg," -d 1000");//add the deadline

						write(FIFOfd,msg,strlen(msg)+1);
						free(msg);

						while( 1 ){
							char *ans = malloc(9*sizeof(char));//should not be resized
							read_command(FIFOfd2,&ans,9);//read jobExec answers
							if(!strcmp(ans,"_STOP_")){free(ans);break;}//no more results
							char *realans=malloc((strlen(ans)+2)*sizeof(char));
							strcpy(realans,ans);
							strcat(realans,"\n");
							write(newcsock,realans,strlen(ans)+2);//write ans to socket
							free(realans);
							free(ans);
						}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
					}
					else
						write(newcsock,"Unknown command!\n",18);
					free(ms);
				}
				free(buf);
				//close(newcsock);//--8
			}
		}//9
		if(flagSHUTDOWN) break;//10
	}

	//clear pool and then place in pool exit msg
	urlpool_clear(&up);
	//here if i got SHUTDOWN command to send exit msg(sending "exit" in pool)
	for(int i=0; i<n_threads; i++){
		place(&up,"exit");
		pthread_cond_signal(&cv_notempty);
		printf("Parent: send stop!\n");
	}

	for(int i=0; i<n_threads; i++)
		 pthread_join(*(tids+i),NULL);//join threads

	if( !flagSEARCH ){//free struct from search
		write(FIFOfd,"/exit",6);
		close(FIFOfd);
		close(FIFOfd2);
	}

	//free structs
	pthread_mutex_destroy(&mtx);
	pthread_mutex_destroy(&mtxwt);
	pthread_mutex_destroy(&pumtx);
	pthread_mutex_destroy(&mtxcounter);
	pthread_mutex_destroy(&mtxbytes);
	pthread_cond_destroy(&cv_notempty);
	pthread_cond_destroy(&cv_notfull);
	urlpool_destroy(&up);
	destroyPU();
	free(dat->savedir);
	free(dat->hostname);
	free(savedir);
	free(hostid);
	free(tids);
	free(dat);
	if( !flagSEARCH ){//free other structs first to give jE some
		waitpid(pid,0,0);//time to exit and then
		unlink("../JECfifo");//unlink fifos
		unlink("../CJEfifo");
	}
	printf("Crawler about to exit!\n");
	return 0;
}


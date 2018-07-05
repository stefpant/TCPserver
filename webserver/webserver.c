#ifndef _GNU_SOURCE //define for accept4 function
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
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<time.h>
#include<poll.h>
#include<fcntl.h>

#include"functions.h"
#include"input_parse.h"

#define perror2(s,e) fprintf(stderr, "%s: %s\n", s, strerror(e))
#define MAXR 50 //pool size
#define BUFFERSIZE 128 //buffer size to read requests from clients

pthread_mutex_t mtx;
pthread_mutex_t mtxcounter;
pthread_mutex_t mtxbytes;
pthread_cond_t cv_notempty;
pthread_cond_t cv_notfull;
int totalrc = 0;//for command STATS
int totalBytes = 0;//for command STATS
typedef struct {
	int* fds;//array of file descriptors
	int count;
	int start;
	int end;
} reqArray;//pool of fds

reqArray *ra;

void reqArray_init(reqArray** ra){
	(*ra) = malloc(sizeof(reqArray));
	(*ra)->fds = malloc(MAXR*sizeof(int));
	(*ra)->start = 0;
	(*ra)->end = -1;
	(*ra)->count = 0;
}

void reqArray_destroy(reqArray** ra){
	free((*ra)->fds);
	free((*ra));
}

void place(reqArray** ra,int fd){// place request in buffer
	pthread_mutex_lock(&mtx);
	while((*ra)->count>=MAXR){
		//printf("Pool found full\n");
		pthread_cond_wait(&cv_notfull,&mtx);
	}
	(*ra)->end = ((*ra)->end+1)%MAXR;
	(*ra)->fds[(*ra)->end] = fd;
	//printf("placed: %d\n",(*ra)->fds[(*ra)->end]);
	(*ra)->count++;
	pthread_mutex_unlock(&mtx);

}

int obtain(reqArray** ra){//get request from buffer
	int data;
	pthread_mutex_lock(&mtx);
	while( (*ra)->count <= 0 ){
		//printf("Pool found empty\n");
		pthread_cond_wait(&cv_notempty,&mtx);
	}
	data = (*ra)->fds[(*ra)->start];
	(*ra)->start = ( (*ra)->start + 1 ) % MAXR;
	(*ra)->count--;
	pthread_mutex_unlock(&mtx);
	return data;
}

void *threadExec(void * rootdir){
	char *buf;
	char *page = NULL;
	char *ans = NULL;
	int anslen;
	char *rdir = (char *)rootdir;
	char *rdpage = NULL;
	int reqerr,localbytes;
	while( 1 ){//try to obtain a file desc and exit if u obtain -1
		buf = NULL;//malloc(10*sizeof(char));
		int data;
		data = obtain(&ra);
		//printf("thread: %d\n",data);
		pthread_cond_signal(&cv_notfull);
		if( data == -1) break;
		pthread_mutex_lock(&mtxcounter);
		totalrc++;
		pthread_mutex_unlock(&mtxcounter);
		while( (buf = read_request(data,BUFFERSIZE)) == NULL ){}//in case client haven't answered yet,repeat
		//printf("buf= %s\n",buf);
		reqerr = analyze_request(buf,&page);
		if(page != NULL){
			rdpage = (char *)malloc((strlen(page) + strlen(rdir) + 1)*sizeof(char));
			strcpy(rdpage,rdir);
			strcat(rdpage,page);
		}
		//printf("%s\n",rdpage);
		ans = create_answer(&anslen,rdpage,reqerr,&localbytes);
		pthread_mutex_lock(&mtxbytes);
		totalBytes += localbytes;
		pthread_mutex_unlock(&mtxbytes);
		//printf("%s\n",ans);
		write(data,ans,anslen);
		close(data);
		free(page);
		free(ans);
		free(rdpage);
		free(buf);
	}
	free(buf);
	printf("thread exiting...\n");
	return NULL;
}

//test thread
void* killthread(void * ptr){
	sleep(5);
	return NULL;
}

int main(int argc,char* argv[]){
	//int err;//thread error
	int s_port,c_port;
	int n_threads = 5;//by default 5 threads
	char *rootdir = NULL;
//	int flagSHUTDOWN=0;//when set 1 exit
//~~~~~~~~~~~~~~~~~~~~~~~~~~PARSE ARGUMENTS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	printf("Parent %ld: just started!\n", pthread_self());
	if(input_parse(argc,argv,&s_port,&c_port,&n_threads,&rootdir) < 0){
		free(rootdir);
		exit(9);
	}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//printf("%s\n",rootdir);
//~~~~~~~~~~~~~~~~~~~~~INIT: pool,mutexes,conditions~~~~~~~~~~~~~~~~~~~~~~~
	reqArray_init(&ra);

	pthread_mutex_init(&mtx, 0);
	pthread_mutex_init(&mtxcounter, 0);
	pthread_mutex_init(&mtxbytes, 0);
	pthread_cond_init(&cv_notempty, 0);
	pthread_cond_init(&cv_notfull, 0);
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~MAKING THREADS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	pthread_t *tids = NULL;

	tids = malloc(n_threads*sizeof(pthread_t));

	for(int i=0; i<n_threads; i++)//create threads to execute threadExec
		pthread_create(tids+i,NULL,threadExec, (void *)rootdir);

	//test terminating last thread to create new one
	//to test this decrease above for loop by one( i<nthreads-1 )
	//pthread_create(tids+(n_threads-1),NULL,killthread, (void *)rootdir);

	printf("Parent %ld: just created new threads!\n", pthread_self());
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	time_t start,localend;
	int hh,mm,ss;
	time(&start);//server started
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~MAKING SOCKETS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	int s_sock,c_sock,newsock,newcsock;
	struct sockaddr_in server;
	struct sockaddr * serverptr = (struct sockaddr *)&server;
	if( (s_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0 ) //socket for service port
		{perror("socket");exit(9);}
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(s_port);
	if( setsockopt(s_sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 },sizeof(int)) < 0 )
		{perror("setsockopt");exit(9);}
	if( bind(s_sock,serverptr,sizeof(server)) < 0 ){perror("bind");exit(9);}
	if( listen(s_sock,100) < 0 ){perror("listen");exit(9);}

	if( (c_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) //socket for command port
		{perror("socket");exit(9);}
	server.sin_port = htons(c_port); //change port
	if( setsockopt(c_sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 },sizeof(int)) < 0 )
		{perror("setsockopt");exit(9);}
	if( bind(c_sock,serverptr,sizeof(server)) < 0 ){perror("bind");exit(9);}
	if( listen(c_sock,100) < 0 ){perror("listen");exit(9);}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	struct pollfd pfds[2];
	pfds[0].fd = s_sock;
	pfds[0].events = POLLIN;
	pfds[1].fd = c_sock;
	pfds[1].events = POLLIN;

//~~~~~~~~~~~~~~~~~~~~~~~~~LOOP:ACCEPTING CONNECTIONS~~~~~~~~~~~~~~~~~~~~~~~
	while( 1 ){
//~~~~~~~~~~~~~~~~~~~~~~~~~CHECK IF THREADS ARE ALIVE~~~~~~~~~~~~~~~~~~~~~~~
		for(int i=0; i<n_threads; i++){
			if(pthread_kill(*(tids+i),0) != 0){//thread i died
				printf("Thread with id '%ld' died.\n",*(tids+i));
				pthread_create(tids+i,NULL,threadExec, (void *)rootdir);
				printf("Just created a new one with id '%ld'!\n",*(tids+i));
			}
		}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		poll(pfds,2,-1);//wait a connection
		if(pfds[0].revents & POLLIN){//client connected on service port
			if( (newsock = accept4(s_sock,NULL,NULL,SOCK_NONBLOCK)) < 0)//non blocking fd
				{perror("accept");exit(9);}
			place(&ra, newsock);
			pthread_cond_signal(&cv_notempty);
			//printf("Parent: send fd!\n");
		}
		if(pfds[1].revents & POLLIN){//client on command port
			if( (newcsock = accept(c_sock,NULL,NULL)) < 0)
				{perror("accept");exit(9);}
//			while(1){//1
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
					if(asprintf(&statrep,"Server up for %02d:%02d:%02d, served %d pages, %d bytes\n",hh,mm,ss,totalrc,totalBytes) != -1){
						int len=strlen(statrep);
						write(newcsock,statrep,len+1);
						free(statrep);
					}
					//printf("Server up for %02d:%02d:%02d, served %d pages, %d bytes\n",hh,mm,ss,totalrc,totalBytes);
				}
				else if( !strcmp(buf,"SHUTDOWN") ){
					close(newcsock);
					free(buf);
//					flagSHUTDOWN=1;//2
					break;
				}
//				else if( !strcmp(buf,"STOP") ){//3
//					close(newcsock);//4
//					free(buf);//5
//					break;//6
//				}//7
				else
					write(newcsock,"Unknown command!\n",18);
				free(buf);
				close(newcsock);//--8
//			}//9
		}
//		if(flagSHUTDOWN) break;//10
	}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//should clear pool first
	//here if i got SHUTDOWN command to send exit msg(sending -1 in pool)
	for(int i=0; i<n_threads; i++){
		place(&ra,-1);
		pthread_cond_signal(&cv_notempty);
		printf("Parent: send stop!\n");
	}

	for(int i=0; i<n_threads; i++)
		 pthread_join(*(tids+i),NULL);//join threads

	//printf("Finaly counter is %d\n",totalrc);

	//free structs
	pthread_mutex_destroy(&mtx);
	pthread_mutex_destroy(&mtxcounter);
	pthread_mutex_destroy(&mtxbytes);
	pthread_cond_destroy(&cv_notempty);
	pthread_cond_destroy(&cv_notfull);
	reqArray_destroy(&ra);
	close(s_sock);
	close(c_sock);
	free(rootdir);
	free(tids);
	return 0;
}


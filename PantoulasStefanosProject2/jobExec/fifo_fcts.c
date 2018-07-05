#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"functions.h"
#include"fifo_fcts.h"

void initNames(fifo_info **fifo,int n){
	if(fifo==NULL) return ;
	char *f1="../PantoulasStefanosProject2/jobExec/tmp/fifo1_w";
	char *f2="../PantoulasStefanosProject2/jobExec/tmp/fifo2_w";
	char *buf;
	for(int i=0;i<n;i++){
		buf = myitoa(i+1);
		(*fifo)[2*i].name = malloc((49 + itol(i+1))*sizeof(char));
		strcpy((*fifo)[2*i].name,f1);
		strcat((*fifo)[2*i].name,buf);
		(*fifo)[2*i+1].name = malloc((49 + itol(i+1))*sizeof(char));
		strcpy((*fifo)[2*i+1].name,f2);
		strcat((*fifo)[2*i+1].name,buf);
		free(buf);
	}
}

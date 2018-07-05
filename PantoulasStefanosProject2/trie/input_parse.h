#ifndef INPUT_PARSE
#define INPUT_PARSE

//int doc_parse(char** DocContent[],char* docfile,int *lines);
int read_doc(char *name,char ***map,int *lines);
int input_parse(int argc,char* argv[],char ***docmap,int* w,int* sorted,int *lines,char** fifoname,char** fifoname2);

#endif

CC = gcc
FLAGS = -Wall
OBJECTS1 = webserver/functions.o webserver/input_parse.o webserver/webserver.o
OBJECTS2 = webcrawler/functions.o webcrawler/input_parse.o webcrawler/webcrawler.o
OBJECTS3 = PantoulasStefanosProject2/trie/input_parse.o  PantoulasStefanosProject2/trie/postingList.o PantoulasStefanosProject2/trie/trie.o PantoulasStefanosProject2/trie/trieInsertion.o PantoulasStefanosProject2/jobExec/child.o PantoulasStefanosProject2/jobExec/fifo_fcts.o PantoulasStefanosProject2/jobExec/functions.o PantoulasStefanosProject2/jobExec/jobExecutor.o PantoulasStefanosProject2/jobExec/parent.o PantoulasStefanosProject2/jobExec/PCOptions.o


all : main1 main2 main3

main1 : $(OBJECTS1)
	$(CC) $(FLAGS) -o ./webserver/myhttpd $(OBJECTS1) -lpthread

main2 : $(OBJECTS2)
	$(CC) $(FLAGS) -o ./webcrawler/mycrawler $(OBJECTS2) -lpthread

main3 : $(OBJECTS3)
	$(CC) $(FLAGS) -o ./PantoulasStefanosProject2/jobExecutor $(OBJECTS3) -lm

webserver/functions.o : webserver/functions.c
	$(CC) $(FLAGS) -c webserver/functions.c -o webserver/functions.o

webserver/input_parse.o : webserver/input_parse.c
	$(CC) $(FLAGS) -c webserver/input_parse.c -o webserver/input_parse.o

webserver/webserver.o : webserver/webserver.c
	$(CC) $(FLAGS) -c webserver/webserver.c -o webserver/webserver.o

webcrawler/functions.o : webcrawler/functions.c
	$(CC) $(FLAGS) -c webcrawler/functions.c -o webcrawler/functions.o

webcrawler/input_parse.o : webcrawler/input_parse.c
	$(CC) $(FLAGS) -c webcrawler/input_parse.c -o webcrawler/input_parse.o

webcrawler/webcrawler.o : webcrawler/webcrawler.c
	$(CC) $(FLAGS) -c webcrawler/webcrawler.c -o webcrawler/webcrawler.o

PantoulasStefanosProject2/trie/input_parse.o : PantoulasStefanosProject2/trie/input_parse.c
	$(CC) $(FLAGS) -c PantoulasStefanosProject2/trie/input_parse.c -o PantoulasStefanosProject2/trie/input_parse.o

PantoulasStefanosProject2/trie/postingList.o : PantoulasStefanosProject2/trie/postingList.c
	$(CC) $(FLAGS) -c PantoulasStefanosProject2/trie/postingList.c -o PantoulasStefanosProject2/trie/postingList.o

PantoulasStefanosProject2/trie/trie.o : PantoulasStefanosProject2/trie/trie.c
	$(CC) $(FLAGS) -c PantoulasStefanosProject2/trie/trie.c -o PantoulasStefanosProject2/trie/trie.o

PantoulasStefanosProject2/trie/trieInsertion.o : PantoulasStefanosProject2/trie/trieInsertion.c
	$(CC) $(FLAGS) -c PantoulasStefanosProject2/trie/trieInsertion.c -o PantoulasStefanosProject2/trie/trieInsertion.o

PantoulasStefanosProject2/jobExec/child.o : PantoulasStefanosProject2/jobExec/child.c
	$(CC) $(FLAGS) -c PantoulasStefanosProject2/jobExec/child.c -o PantoulasStefanosProject2/jobExec/child.o

PantoulasStefanosProject2/jobExec/fifo_fcts.o : PantoulasStefanosProject2/jobExec/fifo_fcts.c
	$(CC) $(FLAGS) -c PantoulasStefanosProject2/jobExec/fifo_fcts.c -o PantoulasStefanosProject2/jobExec/fifo_fcts.o

PantoulasStefanosProject2/jobExec/functions.o : PantoulasStefanosProject2/jobExec/functions.c
	$(CC) $(FLAGS) -c PantoulasStefanosProject2/jobExec/functions.c -o PantoulasStefanosProject2/jobExec/functions.o

PantoulasStefanosProject2/jobExec/jobExecutor.o : PantoulasStefanosProject2/jobExec/jobExecutor.c
	$(CC) $(FLAGS) -c PantoulasStefanosProject2/jobExec/jobExecutor.c -o PantoulasStefanosProject2/jobExec/jobExecutor.o

PantoulasStefanosProject2/jobExec/parent.o : PantoulasStefanosProject2/jobExec/parent.c
	$(CC) $(FLAGS) -c PantoulasStefanosProject2/jobExec/parent.c -o PantoulasStefanosProject2/jobExec/parent.o

PantoulasStefanosProject2/jobExec/PCOptions.o : PantoulasStefanosProject2/jobExec/PCOptions.c
	$(CC) $(FLAGS) -c PantoulasStefanosProject2/jobExec/PCOptions.c -o PantoulasStefanosProject2/jobExec/PCOptions.o

.PHONY : clean
clean :
	rm $(OBJECTS1) $(OBJECTS2) $(OBJECTS3) ./webserver/myhttpd ./webcrawler/mycrawler ./PantoulasStefanosProject2/jobExecutor

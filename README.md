# TCPserver

Project seperated in 3 parts:


    1. --Webcreator--:
        Bash script that help us initialize a root directory for our server with w sites and p pages.
        Also every page contains internal/external links to other pages in same/different site.
        Run: ./webcreator.sh ./root_dir leagues_jv_sea.txt w p

    2. --Webserver--:
        HTTP Server that serves clients who request pages of root directory.Uses sockets to listen to 2 ports.
        In service port waits for http requests and lets other threads that has created to replay.
        In command port whats for user commands(like **STATS** to give statistics for running time and served
        requests and **SHUTDOWN** to terminate).
        Run: ./myhttpd -p <service_port> -c <command_port> -d ../root_dir -t <num_of_threads>
        (better to run in webserver directory)

    3. --Webcrawler--:
        Starting from one URL(aka page from root dir) tries to request all pages and in the end copy root
        directory to save_dir.At the same time,on command port waits for user commands(same as webserver
        and a new SEARCH command(typing "SEARCH word1 ... [max]word10") to search for any of those words in
        save_dir(forking a new process and running jobExecutor)).

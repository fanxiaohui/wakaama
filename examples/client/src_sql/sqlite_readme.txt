
#the sqlite3 src code is from below url
#https://www.sqlite.org/download.html , sqlite-amalgamation-xxxx.zip, which include 4 files( shell.c  sqlite3.c  sqlite3ext.h  sqlite3.h), shell.c is used for CLI(command line tool), others need by lwm2m project;

#https://www.sqlite.org/howtocompile.html
#https://www.sqlite.org/quickstart.html

#https://www.sqlite.org/cintro.html   An Introduction To The SQLite C/C++ Interface 

#https://www.sqlite.org/cli.html   Command Line Shell For SQLite  ; 

#gcc shell.c sqlite3.c -lpthread -ldl

gcc -Os -I.  -DSQLITE_ENABLE_FTS4 \
    -DSQLITE_ENABLE_JSON1 \
   -DSQLITE_ENABLE_RTREE  \
   -DHAVE_USLEEP  \
   shell.c sqlite3.c -ldl -lpthread  -o sqlite3

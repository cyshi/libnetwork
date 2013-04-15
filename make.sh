#gcc -g -O0 -fprofile-arcs -ftest-coverage --coverage -lgcov -Wall -lpthread main.c  buff.c recover.c socket.c thread.c fyutils.c connection.c server.c notify.c && ./a.out
gcc -g -O2 -Wall -lpthread main.c  buff.c recover.c socket.c thread.c fyutils.c connection.c server.c notify.c && ./a.out




CC = gcc

LIBS = -lresolv -lnsl -lpthread\
	/home/nilesh/Documents/assgn2/libunp.a\
	
FLAGS = -g -O2

CFLAGS = ${FLAGS} -I/home/nilesh/Documents/assgn2/lib

PROGS = client server 

all: ${PROGS}

client: client.o get_ifi_info_plus.o
	${CC} ${FLAGS} -o client client.o get_ifi_info_plus.o ${LIBS}
client.o: client.c
	${CC} ${CFLAGS} -c client.c
	
server: server.o get_ifi_info_plus.o sigchldwait.o
	${CC} ${FLAGS} -o server server.o get_ifi_info_plus.o sigchldwait.o ${LIBS}
server.o: server.c
	${CC} ${CFLAGS} -c server.c

test: test.o
	${CC} ${FLAGS} -o test test.o ${LIBS}
	
test.o: test.c
	${CC} ${CFLAGS} -c test.c	
	
get_ifi_info_plus.o: get_ifi_info_plus.c
	${CC} ${CFLAGS} -c get_ifi_info_plus.c	
	
sigchldwait.o: sigchldwait.c
	${CC} ${CFLAGS} -c sigchldwait.c
clean:
	rm client server *.o

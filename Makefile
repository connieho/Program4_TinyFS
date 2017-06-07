CC = gcc

all: tinyFsDemo

tinyFsDemo: tinyFsDemo.c libDisk.o libTinyFS.o 
	$(CC) -o tinyFsDemo tinyFsDemo.c

libDisk.o: libDisk.c libDisk.h tinyFS_errno.h tinyFS.h
	$(CC) -c libDisk.c

libTinyFS.o: tinyFS.h libTinyFS.c libTinyFS.h tinyFS_errno.h
	$(CC) -c libTinyFS.c
   
clean:
	rm -f tinyFsDemo *.o

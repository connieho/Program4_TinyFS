CC = gcc

all: tinyFsDemo

tinyFsDemo: tinyFsDemo.c 
	$(CC) 

libDisk: libDisk.c libDisk.h
	$(CC) -o libDisk libDisk.c 

libTinyFS: libTinyFS.c libTinyFS.h
	$(CC) -o lib

clean:
	rm -f tinyFsDemo *.o

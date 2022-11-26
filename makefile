all: mfs

mfs: mfs.o filesystem.o
	gcc -std=c99 -o mfs mfs.o filesystem.o

mfs.o: mfs.c
	gcc -std=c99 -Wall -c mfs.c

filesystem.o: filesystem.c filesystem.h
	gcc -std=c99 -Wall -c filesystem.c

fcopy: block_copy_example.c
	gcc -std=c99 -o fcopy block_copy_example.c

clean:
	rm mfs
	rm *.o

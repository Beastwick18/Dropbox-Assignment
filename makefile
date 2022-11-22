all: mfs

mfs: mfs.o filesystem.o
	gcc -o mfs mfs.o filesystem.o

mfs.o: mfs.c
	gcc -Wall -c mfs.c

filesystem.o: filesystem.c filesystem.h
	gcc -Wall -c filesystem.c

fcopy: block_copy_example.c
	gcc -o fcopy block_copy_example.c

clean:
	rm mfs

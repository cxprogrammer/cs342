
all: fat32

fat32: fat32.c
	gcc -Wall -g -o fat32 fat32.c

push:
	git push https://github.com/korpeoglu/cs342-fall2016-p5.git

clean: 	
	rm -fr *~  fat32 a.out

all:
	gcc part1.c -o part1
	./part1 BACKING_STORE.bin addresses.txt

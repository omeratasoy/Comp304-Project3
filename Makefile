1:
	gcc part1.c -o part1
	./part1 BACKING_STORE.bin addresses.txt

fifo2:
	gcc part2.c -o part2
	./part2 BACKING_STORE.bin addresses.txt -p 0

lru2:
	gcc part2.c -o part2
	./part2 BACKING_STORE.bin addresses.txt -p 1

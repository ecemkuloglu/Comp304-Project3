compile:
	gcc part1.c -o part1.o && gcc part2.c -o part2.o

part1:
	gcc part1.c -o part1.o && ./part1.o ./BACKING_STORE.bin addresses.txt

fifo:
	gcc part2.c -o part2.o && ./part2.o ./BACKING_STORE.bin addresses.txt -p 0

lru:
	gcc part2.c -o part2.o && ./part2.o ./BACKING_STORE.bin addresses.txt -p 1


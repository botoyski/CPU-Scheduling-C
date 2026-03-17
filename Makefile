CC=gcc
CFLAGS=-Wall

all:
	$(CC) $(CFLAGS) main.c fcfs.c sjf.c stcf.c simulator.c -o schedsim

run:
	./schedsim

clean:
	rm -f schedsim
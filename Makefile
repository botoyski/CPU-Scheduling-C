CC=gcc
CFLAGS=-Wall

all:
	$(CC) $(CFLAGS) main.c fcfs.c sjf.c stcf.c rr.c mlfq.c simulator.c trace.c -o schedsim

run:
	./schedsim

clean:
	rm -f schedsim
CC=gcc
CFLAGS=-Wall

all:
	$(CC) $(CFLAGS) main.c fcfs.c sjf.c stcf.c rr.c mlfq.c simulator.c trace.c queue.c -o schedsim

test-metrics:
	$(CC) $(CFLAGS) test_metrics.c simulator.c trace.c -o test_metrics
	./test_metrics

run:
	./schedsim

clean:
	rm -f schedsim test_metrics
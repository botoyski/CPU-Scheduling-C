CC=gcc
CFLAGS=-Wall -I./include -I. -I..

all:
	$(CC) $(CFLAGS) src/main.c src/cli.c src/scheduler.c src/fcfs.c src/sjf.c src/stcf.c src/rr.c src/mlfq.c src/process.c src/metrics.c src/gantt.c src/utils.c src/queue.c -o schedsim

test-metrics:
	$(CC) $(CFLAGS) test/test_metrics.c src/metrics.c src/gantt.c -o test_metrics
	./test_metrics

test-queue:
	$(CC) $(CFLAGS) test/test_queue.c src/queue.c -o test_queue
	./test_queue

run:
	./schedsim

clean:
	rm -f schedsim test_metrics test_queue
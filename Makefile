all:
	gcc  scheduler.c -std=c99 -o scheduler
	gcc  process.c -std=c99 -o process
test:
	sudo ./scheduler

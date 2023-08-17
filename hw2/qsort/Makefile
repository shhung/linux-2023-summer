CFLAGS :=  -Wall -lpthread -DUSE_LINUX
TSAN := -g -fsanitize=thread

qsort: qsort-mt.c atomic.h cond.h futex.h mutex.h spinlock.h
	gcc $(CFLAGS) qsort-mt.c -o qsort-mt 

clean:
	rm qsort
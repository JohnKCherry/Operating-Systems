.PHONY: all clean

all: fork-example tree-example ask2-fork ask2-signals ask2-fork1 ask2-fork2 ask2-fork3 ask2-fork4

CC = gcc
CFLAGS = -g -Wall -O2
SHELL= /bin/bash

tree-example: tree-example.o tree.o
	$(CC) $(CFLAGS) $^ -o $@

fork-example: fork-example.o proc-common.o
	$(CC) $(CFLAGS) $^ -o $@

ask2-fork: ask2-fork.o proc-common.o
	$(CC) $(CFLAGS) $^ -o $@

ask2-signals: ask2-signals.o proc-common.o tree.o
	$(CC) $(CFLAGS) $^ -o $@

ask2-fork1: ask2-fork1.o proc-common.o
	$(CC) $(CFLAGS) $^ -o $@

ask2-fork2: ask2-fork2.o proc-common.o tree.o
	$(CC) $(CFLAGS) $^ -o $@

ask2-fork3: ask2-fork3.o proc-common.o tree.o
	$(CC) $(CFLAGS) $^ -o $@

ask2-fork4: ask2-fork4.o proc-common.o tree.o
	$(CC) $(CFLAGS) $^ -o $@


%.s: %.c
	$(CC) $(CFLAGS) -S -fverbose-asm $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.i: %.c
	gcc -Wall -E $< | indent -kr > $@

clean: 
	rm -f *.o tree-example fork-example pstree-this ask2-{fork,fork1,fork2,fork3,fork4,tree,signals,pipes}

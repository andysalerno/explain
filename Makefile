CC=gcc
CFLAGS=-std=c99

all:
	gcc explain.c -o explain -std=c99 -O3

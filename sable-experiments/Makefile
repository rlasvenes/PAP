.PHONY: default clean

SOURCE	= $(wildcard *.c)
EXEC	= $(SOURCE:.c=)

CC	= gcc
CFLAGS	= -fopenmp -g
LDFLAG	= -fopenmp
LDLIBS	= -lm

default: $(EXEC)

clean:
	rm -f $(EXEC)

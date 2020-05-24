ARCH            := $(shell uname -s | tr a-z A-Z)

CFLAGS		:= -O3 

ifeq ($(ARCH),DARWIN)
LDLIBS		:= -framework OpenCL
else
LDLIBS		:= -lOpenCL
endif

CL_SOURCE	:= $(wildcard *.cl)
OBJECT		:= $(CL_SOURCE:.cl=.o)
EXEC		:= $(CL_SOURCE:.cl=)

CFLAGS		+= -DKERNEL_FILE=\"$(CL_SOURCE)\" -DCL_SILENCE_DEPRECATION

target: $(EXEC)

$(EXEC): util.o

$(OBJECT): util.h Makefile

util.o: util.h Makefile

clean:
	rm -rf $(EXEC) *.o

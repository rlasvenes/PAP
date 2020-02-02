
PROGRAM	:= bin/easyview

# Must be the first rule
.PHONY: default
default: $(PROGRAM)

### config section ###

ENABLE_FUT=1

######################


ARCH := $(shell uname -s | tr a-z A-Z)

SOURCES := $(wildcard src/*.c)

OBJECTS := $(SOURCES:src/%.c=obj/%.o)

DEPENDS := $(SOURCES:src/%.c=deps/%.d)

MAKEFILES := Makefile

CFLAGS += -O3 -Wall -Wno-unused-function #-march=native
CFLAGS += -I./include -I../include
LDFLAGS += -lm

CFLAGS += $(shell pkg-config SDL2_image --cflags)
LDLIBS += $(shell pkg-config SDL2_image SDL2_ttf --libs)

ifdef ENABLE_FUT
CFLAGS += -DENABLE_FUT $(shell pkg-config --cflags fxt)
LDLIBS += $(shell pkg-config --libs fxt)
endif


ifeq ($(ARCH),DARWIN)
LDLIBS		+= -framework OpenGL
else
CFLAGS		+= -rdynamic
LDFLAGS		+= -export-dynamic
LDLIBS		+= -lGL -lpthread -ldl
endif

$(OBJECTS): $(MAKEFILES)

$(PROGRAM): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(OBJECTS): obj/%.o: src/%.c
	$(CC) -o $@ $(CFLAGS) -DTHIS_FILE=\"traces/$<\" -c $<


.PHONY: depend
depend: $(DEPENDS)

$(DEPENDS): $(MAKEFILES)

$(DEPENDS): deps/%.d: src/%.c
	$(CC) $(CFLAGS) -MM $< | \
		sed -e 's|\(.*\)\.o:|deps/\1.d obj/\1.o:|g' > $@

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPENDS)
endif

.PHONY: clean
clean:
	rm -f $(PROGRAM) obj/*.o deps/*.d

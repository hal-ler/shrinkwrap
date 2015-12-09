CC=g++
INCLUDES=-I.
CFLAGS=-c -std=c++0x -g -Wall -Werror -O3
LDFLAGS=-g

SRCS    := $(wildcard *.cpp)
OBJS    := $(patsubst %.cpp,obj/%.o,$(SRCS))
EXES    := $(patsubst %.cpp,%.exe,$(SRCS))
DEPS    := $(OBJS:.o=.d)

VTVCC=g++
VTVFLAGS=-fvtable-verify=std -rdynamic -std=c++0x -O0 -g

all: directories $(EXES)

clean:
	rm obj/*.o
	rm obj/*.d
	rm $(EXE)

%.exe: obj/%.o
	$(CC) $(LDFLAGS) -o $@ $< 

obj/%.o: %.cpp
	$(CC) $(INCLUDES) $(CFLAGS) -MMD -o $@ $<

directories:
	mkdir -p obj

-include $(DEPS)

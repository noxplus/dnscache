TIMESTAMP:=$(shell date +%Y-%m-%d\ %T\ %z)
AUTHER:=$(USER)
TIMEVER:=$(shell date +%s)

BASE:=.
SRC:=$(BASE)/src
INC:=$(BASE)/inc
TMP:=$(BASE)/obj
BIN:=$(BASE)/bin

vpath %.c $(SRC)
vpath %.cpp $(SRC)
vpath %.h $(INC)
vpath %.hpp $(INC)
vpath %.o $(TMP)

ARCH:=

CC:=${ARCH}gcc
CPP:=${ARCH}g++
CFLAG:=-Wall -O2 -I$(INC)
DFLAG:=
#DFLAG:=-DTIMESTAMP="\"$(TIMESTAMP)\"" -DAUTHER="\"$(AUTHER)\"" -DTIMEVER=$(TIMEVER)
LFLAG:=-lpthread

TARGET=util rbtree gghost transdns main

none:
	@echo all target: $(TARGET)
obj:$(addsuffix .o, $(TARGET))
	@echo make all obj-file
bin:$(addsuffix .o, $(TARGET))
	@echo make bin-file
	${CC} -o $(BIN)/dnscache $^ $(LFLAG)

%:%.c util.c
	${CC} -o $(BIN)/$@ $^ -DONLY_RUN $(CFLAG) $(DFLAG)

%.o:%.c
	${CC} -o $(TMP)/$@ -c $< $(CFLAG) $(DFLAG)
%.o:%.cpp
	${CPP} -o $(TMP)/$@ -c $< $(CFLAG) $(DFLAG)

rebuild:clean obj bin

clean:
	@-rm -f $(BIN)/* $(TMP)/*.o

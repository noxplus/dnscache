TIMESTAMP:=$(shell date +%Y-%m-%d\ %T\ %z)
AUTHER?=$(USER)
TIMEVER1?=$(shell date +%Y%m%d)
TIMEVER2?=$(shell date +%H%M%S)
INTVER?=0

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
vpath %.oo $(TMP)

ARCH:=LANGUAGE=C 

CC:=${ARCH}gcc
CPP:=${ARCH}g++
CFLAG:=-Wall -O2 -I$(INC)
ifdef STRONG
	CFLAG+= -Werror
endif
DFLAG:=-DTIMESTAMP="\"$(TIMESTAMP)\"" -DAUTHER="\"$(AUTHER)\"" -DTIMEVER="\"$(TIMEVER1)\"" -DVER=$(INTVER)
LFLAG:=

TARGET=util netsock dnscache gghost

none:
	@echo all target: $(TARGET)

util.o netsock.o dnscache.o gghost.o : util.hpp
dnscache.o netsock.o : netsock.hpp
dnscache.o : netsock.hpp
gghost.o : gghost.hpp

obj : $(addsuffix .o, $(TARGET)) gghost.oo dnscache.oo
	@echo make all obj-file

%.o : %.cpp
	${CPP} -o $(TMP)/$@ -c $< $(CFLAG) $(DFLAG)
%.oo : %.cpp
	${CPP} -o $(TMP)/$@ -c $< $(CFLAG) $(DFLAG) -DONLY_RUN

dnscache : LFLAG+= -lpthread

gghost : gghost.oo netsock.o util.o
	${CPP} -o $(BIN)/$@ $^ $(LFLAG)
dnscache : dnscache.oo netsock.o util.o
	${CPP} -o $(BIN)/$@ $^ $(LFLAG)

rebuild : clean obj bin

clean:
	@-rm -f $(BIN)/* $(TMP)/*.o $(TMP)/*.oo

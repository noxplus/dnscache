TIMESTAMP:=$(shell date +%Y-%m-%d\ %T\ %z)
AUTHER:=$(USER)
TIMEVER1:=$(shell date +%Y%m%d)
TIMEVER2:=$(shell date +%s)

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
DFLAG:=
#DFLAG:=-DTIMESTAMP="\"$(TIMESTAMP)\"" -DAUTHER="\"$(AUTHER)\"" -DTIMEVER=$(TIMEVER)
LFLAG:=

TARGET=util netsock dnsutil gghost main

none:
	@echo all target: $(TARGET)

util.o netsock.o dnsutil.o gghost.o : util.hpp
dnsutil.o netsock.o : netsock.hpp
dnsutil.o : netsock.hpp
gghost.o : gghost.hpp

obj : $(addsuffix .o, $(TARGET))
	@echo make all obj-file
bin : $(addsuffix .o, $(TARGET))
	@echo make bin-file
	${CC} -o $(BIN)/dnscache $^ $(LFLAG)

%.o : %.cpp
	${CPP} -o $(TMP)/$@ -c $< $(CFLAG) $(DFLAG)
%.oo : %.cpp
	${CPP} -o $(TMP)/$@ -c $< $(CFLAG) $(DFLAG) -DONLY_RUN

gghost : gghost.oo netsock.o util.o
	${CPP} -o $(BIN)/$@ $^ $(LFLAG)
dnsutil : dnsutil.oo netsock.o util.o
	${CPP} -o $(BIN)/$@ $^ $(LFLAG)

rebuild : clean obj bin

clean:
	@-rm -f $(BIN)/* $(TMP)/*.o

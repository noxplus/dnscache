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

ARCH:=LANGUAGE=C 

CC:=${ARCH}gcc
CPP:=${ARCH}g++
CFLAG:=-Wall -O2 -I$(INC)
DFLAG:=
#DFLAG:=-DTIMESTAMP="\"$(TIMESTAMP)\"" -DAUTHER="\"$(AUTHER)\"" -DTIMEVER=$(TIMEVER)
LFLAG:=

TARGET=util netsock gghost

none:
	@echo all target: $(TARGET)
obj : $(addsuffix .o, $(TARGET))
	@echo make all obj-file
bin : $(addsuffix .o, $(TARGET))
	@echo make bin-file
	${CC} -o $(BIN)/dnscache $^ $(LFLAG)
util.o netsock.o gghost.o : util.hpp

main.o : main.cpp
	${CPP} -o $(TMP)/$@ -c $< $(CFLAG) $(DFLAG)

%.o : %.cpp %.hpp
	${CPP} -o $(TMP)/$@ -c $< $(CFLAG) $(DFLAG)

gghost : gghost.cpp netsock.cpp util.cpp
	${CPP} -o $(BIN)/$@ $^ $(CFLAG) $(DFLAG) $(LFLAG) -DONLY_RUN

rebuild : clean obj bin

clean:
	@-rm -f $(BIN)/* $(TMP)/*.o

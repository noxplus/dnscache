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
vpath %.o $(TMP)

CFLAG:=-Wall -O2 -I$(INC)
DFLAG:=-DTIMESTAMP="\"$(TIMESTAMP)\"" -DAUTHER="\"$(AUTHER)\"" -DTIMEVER=$(TIMEVER)

TARGET=util rbtree testgg transdns src

none:
	@echo all target: $(TARGET)
obj:$(addsuffix .o, $(TARGET))
	@echo make all obj-file
bin:$(addsuffix .o, $(TARGET))
	@echo make bin-file
	gcc -o $(BIN)/dnscache $< $(LFLAG)

%:%.c inc.h
	gcc -o $@ $< -DONLY_RUN $(CFLAG) $(DFLAG)

%.o:%.c inc.h
	gcc -o $(TMP)/$@ -c $< $(CFLAG) $(DFLAG)
%.o:%.cpp inc.h
	g++ -o $(TMP)/$@ -c $< $(CFLAG) $(DFLAG)

rebuild:clean obj bin

clean:
	@-rm -f $(BIN)/* $(TMP)/*.o

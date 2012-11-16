TIMESTAMP:=$(shell date +%Y-%m-%d\ %T\ %z)
AUTHER:=$(USER)
TIMEVER:=$(shell date +%s)

BASE:=.
SRC:=$(BASE)/src
INC:=$(BASE)/inc
OBJ:=$(BASE)/obj
BIN:=$(BASE)/bin

vpath %.c $(SRC)
vpath %.cpp $(SRC)
vpath %.h $(INC)
vpath %.o $(OBJ)

CFLAG:=-Wall -O2 -I$(INC)
DFLAG:=-DTIMESTAMP="\"$(TIMESTAMP)\"" -DAUTHER="\"$(AUTHER)\"" -DTIMEVER=$(TIMEVER)

target=tgg transdns rbtree

none:
	@echo all target $(target)
obj:$(addsuffix .o, $(target))
	@echo make all
bin:$(target)
	@echo make all target

%.o:%.c
	gcc -o $(OBJ)/$@ -c $^ $(CFLAG) $(DFLAG)
%.o:%.cpp
	g++ -o $(OBJ)/$@ -c $^ $(CFLAG) $(DFLAG)

%:%.o
	gcc -o $(BIN)/$@ $^

rebuild:clean obj bin

clean:
	@-rm -f $(BIN)/* $(OBJ)/*.o

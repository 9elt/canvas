BIN=bin

CODEDIRS=. lib
INCLUDES=-I. -I./lib
LIBS=-lraylib -lm

CC=gcc
OPT=-O0

DEP_FLAGS=-MP -MD -DDEBUG
CFLAGS=-Wall -Wextra -g $(INCLUDES) $(OPT) $(DEP_FLAGS)

CFILES=$(foreach DIR,$(CODEDIRS),$(wildcard $(DIR)/*.c))
OFILES=$(patsubst %.c,%.o,$(CFILES))
DFILES=$(patsubst %.c,%.d,$(CFILES))

all:$(BIN)

$(BIN):$(OFILES)
	$(CC) -o $@ $^ $(LIBS)

%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $< $(LIBS)

clean:
	rm -rf $(BIN) $(OFILES) $(DFILES)

-include $(DFILES)

BIN:=mqtt_sub
LDFLAGS:=-lmosquitto\
-lubus\
-lubox\
-lblobmsg_json\
-luci\
-lcurl

CPPFLAGS:=-I$(CURDIR)
SRC:=$(wildcard *.c)
OBJS:=$(SRC:.c=.o)

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BIN) $(OBJS)

# CFLAGS = -g #-DMONDEBUG

#
# undefine HI_INTS if not on x86 SMP or alpha
#
CFLAGS	+= -W -Wall -pedantic -DHI_INTS \
	   $(shell pkg-config x11     --cflags) \
	   $(shell pkg-config xext    --cflags) \
	   $(shell pkg-config xpm     --cflags)

LIBS	+= $(shell pkg-config x11     --libs) \
	   $(shell pkg-config xext    --libs) \
	   $(shell pkg-config xpm     --libs) \
	   -lm

PREFIX	= /usr

BIN	= wmsysmon
OBJS	= wmgeneral.o \
	  wmsysmon.o  \
	  cpu_linux.o

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(BIN) $(LIBS)

all: wmsysmon

clean:
	rm -f $(OBJS) $(BIN)

install: all
	install -d             $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(BIN) $(DESTDIR)$(PREFIX)/bin

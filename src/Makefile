# CFLAGS = -g #-DMONDEBUG

#
# undefine HI_INTS if not on x86 SMP or alpha
#
CFLAGS	+= -W -Wall -pedantic -DHI_INTS
LIBS	+= -lXpm -lXext -lX11 -lm

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

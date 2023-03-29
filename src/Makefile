# CFLAGS = -g #-DMONDEBUG

PKG_CONFIG ?= pkg-config

#
# undefine HI_INTS if not on x86 SMP or alpha
#
CFLAGS	+= -W -Wall -pedantic -DHI_INTS \
	   $(shell $(PKG_CONFIG) dockapp --cflags) \
	   $(shell $(PKG_CONFIG) x11     --cflags) \
	   $(shell $(PKG_CONFIG) xext    --cflags) \
	   $(shell $(PKG_CONFIG) xpm     --cflags)

LIBS	+= $(shell $(PKG_CONFIG) dockapp --libs) \
	   $(shell $(PKG_CONFIG) x11     --libs) \
	   $(shell $(PKG_CONFIG) xext    --libs) \
	   $(shell $(PKG_CONFIG) xpm     --libs) \
	   -lm

PREFIX	= /usr

BIN	= wmsysmon
OBJS	= wmsysmon.o \
	  cpu_linux.o

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(BIN) $(LIBS)

all: wmsysmon

clean:
	rm -f $(OBJS) $(BIN)

install: all
	install -d             $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(BIN) $(DESTDIR)$(PREFIX)/bin

LIBS += -lXpm -lXext -lX11 -lm
#CFLAGS = -g #-DMONDEBUG
CFLAGS += -W -Wall -pedantic -DHI_INTS #undefine HI_INTS if not on x86 SMP or alpha

PREFIX = /usr

BIN = wmsysmon
OBJS = wmgeneral.o \
	wmsysmon.o  \
	cpu_linux.o

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $(BIN)

all: wmsysmon

clean:
	rm -f $(OBJS) $(BIN)

install: all
	install -g root -o root $(BIN) $(DESTDIR)$(PREFIX)/bin

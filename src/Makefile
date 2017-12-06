CC = gcc

LIBDIR += -L/usr/X11R6/lib
LDFLAGS += -lXpm -lXext -lX11 -lm -s
#CFLAGS = -g #-DMONDEBUG
CFLAGS += ${GENTOO_CFLAGS} -W -Wall -pedantic -I/usr/X11R6/include #-DHI_INTS #undefine HI_INTS if your x86 SMP or alpha

BIN = wmsysmon
OBJS =	wmgeneral.o \
	wmsysmon.o  \
	cpu_linux.o

$(BIN): $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LIBDIR) $(LDFLAGS)

all: wmsysmon

clean:
	rm -f $(OBJS) $(BIN)

install:
	install -g root -o root $(BIN) /usr/local/bin

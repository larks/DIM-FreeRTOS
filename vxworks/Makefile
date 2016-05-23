#
# UNIX specific makefile
#

COBJS = tcpip.c dtq.c utilities.c
OBJS = $(ODIR)/tcpip.o $(ODIR)/dtq.o $(ODIR)/utilities.o

override CFLAGS += -I..

all:	$(OBJS)

clean:
	/bin/rm -f $(OBJS) 

$(ODIR)/tcpip.o:	tcpip.c ../dim.h
	$(CC) $(CFLAGS) -o $(ODIR)/tcpip.o -c tcpip.c
$(ODIR)/dtq.o:		dtq.c ../dim.h
	$(CC) $(CFLAGS) -o $(ODIR)/dtq.o -c dtq.c
$(ODIR)/utilities.o:	utilities.c ../dim.h
	$(CC) $(CFLAGS) -o $(ODIR)/utilities.o -c utilities.c
$(ODIR)/settime.o:	settime.c ../dim.h
	$(CC) $(CFLAGS) -o $(ODIR)/settime.o -c settime.c














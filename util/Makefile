#
# system independant utils makefile
#

COBJS = open_dns.c dll.c sll.c conn_handler.c swap.c hash.c
OBJS = $(ODIR)/open_dns.o $(ODIR)/dll.o $(ODIR)/sll.o $(ODIR)/conn_handler.o $(ODIR)/swap.o $(ODIR)/hash.o

override CFLAGS += -I.. 

all:	$(OBJS)

clean:
	/bin/rm -f $(OBJS) 

$(ODIR)/open_dns.o:	open_dns.c ../dim.h
	$(CC) $(CFLAGS) -o $(ODIR)/open_dns.o -c open_dns.c
$(ODIR)/sll.o:		sll.c ../dim.h
	$(CC) $(CFLAGS) -o $(ODIR)/sll.o -c sll.c
$(ODIR)/dll.o:		dll.c ../dim.h
	$(CC) $(CFLAGS) -o $(ODIR)/dll.o -c dll.c
$(ODIR)/conn_handler.o:	conn_handler.c ../dim.h
	$(CC) $(CFLAGS) -o $(ODIR)/conn_handler.o -c conn_handler.c
$(ODIR)/swap.o:	swap.c
	$(CC) $(CFLAGS) -o $(ODIR)/swap.o -c swap.c
$(ODIR)/hash.o:		hash.c
	$(CC) $(CFLAGS) -o $(ODIR)/hash.o -c hash.c

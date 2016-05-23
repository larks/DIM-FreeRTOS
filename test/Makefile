#
# dna makefile
#

override CFLAGS +=  -I.. 

all:	$(ODIR)/test_server $(ODIR)/test_client $(ODIR)/time_server $(ODIR)/time_client $(ODIR)/sercli

$(DIMDIR)/$(ODIR)/libdim.a:	
	cd ..; make $(ODIR)/libdim.a

$(ODIR)/test_server:	test_server.c $(DIMDIR)/$(ODIR)/libdim.a
	$(CC) $(CFLAGS) $(LDFLAGS) test_server.c -L$(DIMDIR)/$(ODIR) -ldim -o $(ODIR)/test_server $(LIBS)
	
$(ODIR)/test_client:	test_client.c $(DIMDIR)/$(ODIR)/libdim.a
	$(CC) $(CFLAGS) $(LDFLAGS) test_client.c -L$(DIMDIR)/$(ODIR) -ldim -o $(ODIR)/test_client $(LIBS)
	
$(ODIR)/time_server:	time_server.c $(DIMDIR)/$(ODIR)/libdim.a
	$(CC) $(CFLAGS) $(LDFLAGS) time_server.c -L$(DIMDIR)/$(ODIR) -ldim -o $(ODIR)/time_server $(LIBS)
	
$(ODIR)/time_client:	time_client.c $(DIMDIR)/$(ODIR)/libdim.a
	$(CC) $(CFLAGS) $(LDFLAGS) time_client.c -L$(DIMDIR)/$(ODIR) -ldim -o $(ODIR)/time_client $(LIBS)
	
$(ODIR)/sercli:	sercli.c $(DIMDIR)/$(ODIR)/libdim.a
	$(CC) $(CFLAGS) $(LDFLAGS) sercli.c -L$(DIMDIR)/$(ODIR) -ldim -o $(ODIR)/sercli $(LIBS)

thr_c: thr_c.c ../solaris/libdim.a
	cc -I.. thr_c.c -L../solaris -ldim -lsocket -lnsl -lpthread -lthread -othr_c	
	
thr_s: thr_s.c ../solaris/libdim.a
	cc -I.. thr_s.c -L../solaris -ldim -lsocket -lnsl -lpthread -lthread -othr_s	
	



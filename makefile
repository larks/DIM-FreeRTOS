OBJECTS_DIM=conn_handler.o copy_swap.o diccpp.o dic.o dimcpp.o dim_thr.o discpp.o dis.o dll.o dna.o dtq.o hash.o open_dns.o sll.o swap.o tcpip.o tokenstring.o utilities.o
SOURCES_DIM=conn_handler.c copy_swap.c dic.c diccpp.cxx dimcpp.cxx dim_thr.c dis.c discpp.cxx dll.c dna.c dtq.c hash.c open_dns.c sll.c swap.c tcpip.c tokenstring.cxx utilities.c
OBJECTS_DNS=dns.o
SOURCES_DNS=dns.c
EXECUTABLE_DNS=dns

MIPSE=MIPSEL
EXTRAFLAGS=

CFLAGS=-c -fPIC -I. -D$(MIPSE) -DPROTOCOL=1 $(EXTRAFLAGS) 
CC=gcc
CXX=g++
LDFLAGS_DIM=-shared -fPIC -L.
LDFLAGS_DNS=-fPIC -L. -lpthread
EXECUTABLE_DIM=libdim.a
RANLIB=ranlib
LD=ar cru

ifeq ($(ARCH),arm)
CC=arm-uclinuxeabi-gcc
CXX=arm-uclinuxeabi-g++
CFLAGS=-c  -I. -D$(MIPSE) -DPROTOCOL=1 -I${INSTALL_ROOT}/A2F/root/usr/include -g -O2 -mcpu=cortex-m3 -mthumb -L${INSTALL_ROOT}/A2F/root/usr/lib -pthread  
LDFLAGS_DIM=-L. -mcpu=cortex-m3 -mthumb -L${INSTALL_ROOT}/A2F/root/usr/lib -pthread
LDFLAGS_DNS=-L. -mcpu=cortex-m3 -mthumb -L${INSTALL_ROOT}/A2F/root/usr/lib -pthread
LD=arm-uclinuxeabi-ar cru
RANLIB=arm-uclinuxeabi-ranlib
EXECUTABLE_DIM=libdim.a
endif

all: $(EXECUTABLE_DIM) $(EXECUTABLE_DNS)

clean:
	rm -f *.o
	rm -f *.a
	rm -f *.so
	rm -f dns
	rm -f dns.gdb

dns.o: dns.c
	$(CC) $(CFLAGS) -o dns.o dns.c

dns: $(OBJECTS_DNS)
	$(CXX) $(LDFLAGS_DNS) $(OBJECTS_DNS) $(EXECUTABLE_DIM) -o $@

conn_handler.o: conn_handler.c
	$(CC) $(CFLAGS) -o conn_handler.o conn_handler.c

copy_swap.o: copy_swap.c
	$(CC) $(CFLAGS) -o copy_swap.o copy_swap.c

diccpp.o: diccpp.cxx
	$(CXX) $(CFLAGS) -o diccpp.o diccpp.cxx

dic.o: dic.c
	$(CC) $(CFLAGS) -o dic.o dic.c

dimcpp.o: dimcpp.cxx
	$(CXX) $(CFLAGS) -o dimcpp.o dimcpp.cxx

dim_thr.o: dim_thr.c
	$(CC) $(CFLAGS) -o dim_thr.o dim_thr.c

discpp.o: discpp.cxx
	$(CXX) $(CFLAGS) -o discpp.o discpp.cxx

dis.o: dis.c
	$(CC) $(CFLAGS) -o dis.o dis.c

dll.o: dll.c
	$(CC) $(CFLAGS) -o dll.o dll.c

dna.o: dna.c
	$(CC) $(CFLAGS) -o dna.o dna.c

dtq.o: dtq.c
	$(CC) $(CFLAGS) -o dtq.o dtq.c

hash.o: hash.c
	$(CC) $(CFLAGS) -o hash.o hash.c

open_dns.o: open_dns.c
	$(CC) $(CFLAGS) -o open_dns.o open_dns.c

sll.o: sll.c
	$(CC) $(CFLAGS) -o sll.o sll.c

swap.o: swap.c
	$(CC) $(CFLAGS) -o swap.o swap.c

tcpip.o: tcpip.c
	$(CC) $(CFLAGS) -o tcpip.o tcpip.c

tokenstring.o: tokenstring.cxx
	$(CXX) $(CFLAGS) -o tokenstring.o tokenstring.cxx

utilities.o: utilities.c
	$(CC) $(CFLAGS) -o utilities.o utilities.c

libdim.so: $(OBJECTS_DIM)
	$(CXX) $(LDFLAGS_DIM) $(OBJECTS_DIM) -o $@

libdim.a:  $(OBJECTS_DIM)
	$(LD) $(EXECUTABLE_DIM) $(OBJECTS_DIM)
	$(RANLIB) $(EXECUTABLE_DIM)
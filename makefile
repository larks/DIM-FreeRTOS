#
# global makefile
#
CPU              = PPC604
TOOL             = gnu

TARGET_DIR	= mv2604
VENDOR		= Motorola
BOARD		= MVME2600

DEFINE	= -D_GNU_TOOL -DMV2600 -DCPU=PPC604


CC		= ccppc
AR		= arppc
CXX		= g++ppc

INCLUDES	= -I. -I/home/daq/wind_ppc1.0.1/target/config/all \
		-I/home/daq/wind_ppc1.0.1/target/h \
		-I/home/daq/wind_ppc1.0.1/target/src/config \
		-I/home/daq/wind_ppc1.0.1/target/src/drv \
		-I/home/daq/wind_ppc1.0.1/target/config/mv2604


DIMDEFS		= -DMIPSEB -DPROTOCOL=1 -Dunix -DVxWorks

CFLAGS		= -O2 -fvolatile -fno-builtin -fno-for-scope -mstrict-align \
		-ansi -nostdinc -c $(INCLUDES) $(DEFINE) $(DIMDEFS)

C++FLAGS	= -O2 -fvolatile -fno-builtin -fno-for-scope -mstrict-align \
		-Wall -ansi -nostdinc -c $(INCLUDES) $(DEFINE)

CXXFLAGS	= -O2 -fvolatile -fno-builtin -fno-for-scope -mstrict-align \
		-Wall -ansi -nostdinc -c $(INCLUDES) $(DEFINE)


COBJS = dic.c dis.c dna.c
OBJS =  $(ODIR)/dic.o $(ODIR)/dis.o $(ODIR)/dna.o

EXTRALIBS = -L/home/daq/wind_ppc1.0.1/target/lib -lPPC604gnuvx
ODIR = .
RANLIB = echo

all:	$(ODIR)/libdim.a tests
#all:	$(ODIR)/libdim.a 

$(ODIR)/libdim.a:	$(OBJS)
	cd util; $(MAKE) CFLAGS="$(CFLAGS)" CC="$(CC)" ODIR="$(ODIR)
	cd unix; $(MAKE) CFLAGS="$(CFLAGS)" CC="$(CC)" ODIR="$(ODIR)
	$(AR) crv $(ODIR)/libdim.a $(OBJS) util/$(ODIR)/*.o unix/$(ODIR)/*.o
	$(RANLIB) $(ODIR)/libdim.a

$(ODIR)/dns:	$(ODIR)/dns.o $(ODIR)/libdim.a
	$(CC) $(CFLAGS) -L$(ODIR) $(ODIR)/dns.o -ldim -o $(ODIR)/dns $(EXTRALIBS)

tests: 
	cd test ; $(MAKE) CFLAGS="$(CFLAGS)"  CC="$(CC)" ARCH="$(ARCH)" LIBS="$(EXTRALIBS)" ODIR="$(ODIR)"

didd: 
	cd did ; $(MAKE) CFLAGS="$(CFLAGS)"  CC="$(CC)" ARCH="$(ARCH)" LIBS="$(EXTRALIBS)" ODIR="$(ODIR)"


$(ODIR)/dns.o:		dns.c dim.h
	$(CC) $(CFLAGS) -o $(ODIR)/dns.o -c dns.c
$(ODIR)/dis.o:		dis.c dim.h dis.h
	$(CC) $(CFLAGS) -o $(ODIR)/dis.o -c dis.c
$(ODIR)/dic.o:		dic.c dim.h dic.h
	$(CC) $(CFLAGS) -o $(ODIR)/dic.o -c dic.c
$(ODIR)/dna.o:		dna.c dim.h
	$(CC) $(CFLAGS) -o $(ODIR)/dna.o -c dna.c
$(ODIR)/net.o:		net.c dim.h
	$(CC) $(CFLAGS) -o $(ODIR)/net.o -c net.c




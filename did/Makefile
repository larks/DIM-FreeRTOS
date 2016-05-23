
#
# global makefile
#
override CFLAGS += -I.. -I$(MOTIFINC) -I$(X11INC) -L$(MOTIFLIB) -L$(X11LIB)
UILC = /usr/bin/X11/uil
MORE_FLAGS = -lMrm

ifeq ($(OS),SunOS)
	MOTIFINC = /usr/motif/include
	MOTIFLIB = /usr/motif/lib
	X11INC = /usr/X11R5/include/X11
	X11LIB = /usr/X11R5/lib
	UILC = /usr/motif/bin/uil
endif
ifeq ($(OS),Solaris)
	MOTIFINC = /usr/dt/include
	MOTIFLIB = /usr/dt/lib
	X11INC = /usr/openwin/include
	X11LIB = /usr/openwin/lib
#	UILC = /usr/local/motif/bin/uil
#	UILC = /usr/dt/bin/uil
	UILC = uil
endif
ifeq ($(OS),HP-UX)
	MOTIFINC = /usr/include/Motif1.2
	MOTIFLIB = /usr/lib/Motif1.2
	X11INC = /usr/include/X11R5
	X11LIB = /usr/lib/X11R5
	MORE_FLAGS = -l:libMrm.a
endif
ifeq ($(OS),OSF1)
	MOTIFINC = .
	MOTIFLIB = .
	X11INC = .
	X11LIB = .
endif
ifeq ($(OS),AIX)
	MOTIFINC = /usr/include/Motif1.2
	MOTIFLIB = /usr/lib/Motif1.2
	X11INC = /usr/include/X11R5
	X11LIB = /usr/lib/X11R5
endif

all:	$(ODIR)/did $(ODIR)/did.uid

$(ODIR)/dui_util.o:	dui_util.c dui_util.h
	$(CC) $(CFLAGS) -o $(ODIR)/dui_util.o -c dui_util.c

$(ODIR)/did: did.c did.h $(ODIR)/dui_util.o $(DIMDIR)/$(ODIR)/libdim.a
	$(CC) $(CFLAGS) did.c $(ODIR)/dui_util.o $(MORE_FLAGS) -lXm -lXt -lX11 -L$(DIMDIR)/$(ODIR) -ldim $(LIBS) -o $(ODIR)/did

$(ODIR)/did.uid: did.uil
	$(UILC) did.uil -o $(ODIR)/did.uid


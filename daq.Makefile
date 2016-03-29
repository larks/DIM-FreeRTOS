CPU              = PPC604
TOOL             = gnu

TARGET_DIR	= mv2604
VENDOR		= Motorola
BOARD		= MVME2600

DEFINE	= -D_GNU_TOOL -DMV2600 -DCPU=PPC604


CC		= ccppc
CXX		= g++ppc

INCLUDES	= -I. -I/home/daq/wind_ppc1.0.1/target/config/all \
		-I/home/daq/wind_ppc1.0.1/target/h \
		-I/home/daq/wind_ppc1.0.1/target/src/config \
		-I/home/daq/wind_ppc1.0.1/target/src/drv \
		-I/home/daq/wind_ppc1.0.1/target/config/mv2604


CFLAGS		= -O2 -fvolatile -fno-builtin -fno-for-scope -mstrict-align \
		-Wall -ansi -nostdinc -c $(INCLUDES) $(DEFINE)

C++FLAGS	= -O2 -fvolatile -fno-builtin -fno-for-scope -mstrict-align \
		-Wall -ansi -nostdinc -c $(INCLUDES) $(DEFINE)

CXXFLAGS	= -O2 -fvolatile -fno-builtin -fno-for-scope -mstrict-align \
		-Wall -ansi -nostdinc -c $(INCLUDES) $(DEFINE)

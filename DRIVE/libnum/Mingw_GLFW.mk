OBJS=curvefit.o \
	dmatrix.o \
	gaussj.o \
	imatrix.o \
	integrate.o \
	ivector.o \
	ludcmp.o \
	matrix.o \
	nrerror.o \
	ode.o \
	quaternion.o \
	root.o

#OPTG=-O2
OPTG=-g
LEGACY=-fcommon -std=gnu89 -Wno-implicit-int -Wno-implicit-function-declaration -Wno-return-type -Wno-int-conversion -Wno-incompatible-pointer-types
CC=gcc
CFLAGS=$(OPTG) $(LEGACY)

LIBS=../libnum.a

all:	$(LIBS)

clean:
	rm -f *.o $(LIBS)

../libnum.a:     $(OBJS)
	rm -f $@
	ar rv $@ $(OBJS)

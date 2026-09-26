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
CFLAGS=$(OPTG) -m32

LIBS=../libnum.a

all:	$(LIBS)

clean:
	rm -f *.o $(LIBS)

../libnum.a:     $(OBJS)
	rm -f $@
	ar rv $@ $(OBJS)

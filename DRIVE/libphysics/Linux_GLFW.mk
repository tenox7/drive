OBJS=collision.o \
	pobj_state.o \
	pobject.o \
	pobjst_sim.o

LIBS=../libphysics.a

#OPTG=-O2
OPTG=-g
LEGACY=-fcommon -std=gnu89 -Wno-implicit-int -Wno-implicit-function-declaration -Wno-return-type -Wno-int-conversion -Wno-incompatible-pointer-types
CC=gcc
CFLAGS=$(OPTG) -DGLFW -DMAC -I../libnum $(LEGACY)

all:	$(LIBS)

clean:
	rm -f *.o $(LIBS)

../libphysics.a:     $(OBJS)
	rm -f $@
	ar rv $@ $(OBJS)

OBJS=collision.o \
	pobj_state.o \
	pobject.o \
	pobjst_sim.o

LIBS=../libphysics.a

#OPTG=-O2
OPTG=-g
CFLAGS=$(OPTG) -m32 -DMAC -I../libnum

all:	$(LIBS)

clean:
	rm -f *.o $(LIBS)

../libphysics.a:     $(OBJS)
	rm -f $@
	ar rv $@ $(OBJS)

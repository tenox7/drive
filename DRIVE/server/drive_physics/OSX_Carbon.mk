OBJS=ballistic.o \
	car.o \
	common.o \
	gears.o \
	intersect.o \
	plane.o \
	simple.o \
	ufop.o \
	xfighterp.o

LIBS=../../drive_physics.a

#OPTG=-O2
OPTG=-g
CFLAGS=$(OPTG) -m32 -DMAC -I../../libnum -I../../libphysics -I../../include -I../../../HW/Inc -I../objects -I../../libprims

all:	$(LIBS)

clean:
	rm -f *.o $(LIBS)

../../drive_physics.a:     $(OBJS)
	rm -f $@
	ar rv $@ $(OBJS)

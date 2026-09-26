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
LEGACY=-fcommon -std=gnu89 -Wno-implicit-int -Wno-implicit-function-declaration -Wno-return-type -Wno-int-conversion -Wno-incompatible-pointer-types
CC=gcc
CFLAGS=$(OPTG) -DGLFW -DMINGW -I../../libnum -I../../libphysics -I../../include -I../../../HW/Inc -I../objects -I../../libprims $(LEGACY)

all:	$(LIBS)

clean:
	rm -f *.o $(LIBS)

../../drive_physics.a:     $(OBJS)
	rm -f $@
	ar rv $@ $(OBJS)

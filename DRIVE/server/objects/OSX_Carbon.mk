OBJS=FokD7.o T34.o banks.o barn.o bridge.o \
	bump.o checkpt.o conifer.o countach.o curve.o \
	curvegdrail.o curvewall.o deciduous.o fence.o flames.o \
	flat.o graftal.o ground.o guardrail.o hill.o \
	hillroad.o hillxroad.o house.o lamp.o land_mine.o \
	ltube.o mailbox.o mcycle.o minivan.o mound.o \
	obj_list.o obj_utils.o oval.o parkinglot.o pillar.o \
	police.o polyline.o pond.o railroad.o ramp.o \
	road.o rocket_car.o rubble.o sedan.o shell.o \
	sign.o silo.o skyscraper.o sphere.o spiral.o \
	splroad.o stoplight.o teleline.o twistramp.o ufo.o \
	wall.o wormhole.o xfighter.o

LIBS=../../objects.a

#OPTG=-O2
OPTG=-g
CFLAGS=$(OPTG) -m32 -DMAC -I../../libnum -I../../libphysics -I../../include -I../../../HW/Inc -I../../libprims

all:	$(LIBS)

clean:
	rm -f *.o $(LIBS)

../../objects.a:     $(OBJS)
	rm -f $@
	ar rv $@ $(OBJS)

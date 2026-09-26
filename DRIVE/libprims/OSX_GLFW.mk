OBJS=circle.o \
	extrusion.o \
	mesh2strip.o \
	mesh_cone.o \
	mesh_surfrev.o \
	spl_cone.o \
	spl_extr.o \
	spl_sphere.o \
	spl_surfrev.o \
	stacknots.o \
	stackvf.o \
	trim_pln.o \
	utils.o \
	whatstring.o

LIBS=../libprims.a

#OPTG=-O2
OPTG=-g
LEGACY=-fcommon -std=gnu89 -Wno-implicit-int -Wno-implicit-function-declaration -Wno-return-type -Wno-int-conversion -Wno-incompatible-pointer-types -Wno-deprecated-non-prototype -Wno-deprecated-declarations
CC=clang
CFLAGS=$(OPTG) -DGLFW -DMAC -I../libnum -I../include $(LEGACY)

all:	$(LIBS)

clean:
	rm -f *.o $(LIBS)

../libprims.a:     $(OBJS)
	rm -f $@
	ar rv $@ $(OBJS)

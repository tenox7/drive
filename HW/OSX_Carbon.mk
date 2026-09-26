OBJ=Objs/
OBJD=Objs_d/
OPTG=-O 
DBG=-g 
CC=gcc -m32 -DMAC
PNG=../LIBPNG/OBJ32/readpng.o ../LIBPNG/OBJ32/writepng.o

default:
	(cd Common;make "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	(cd Objects;make "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	(cd GUI;make "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	(cd GLES;make -f OSX_Carbon.mk "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	(cd Null;make "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	make -f OSX_Carbon.mk "OBJ=$(OBJ)" libhwgl.a libhwnull.a libhwut.a

test:
	(cd Test;make "SHELL=/bin/bash" "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)" -f OSX_Carbon.mk)

all:	default test gfx2hw

all_debug:	debug y2k test gfx2hw

y2k:	
	(cd Test/y2k ;make "OPTG=$(DBG)" "OBJ=$(OBJ)")


gl:
	make default
	(cd GLES;make -f OSX_Carbon.mk "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	make "OBJ=$(OBJ)" libhwgl.a

debug:
	(cd Common;make "CC=$(CC)" "OPTG=$(DBG)" "OBJ=$(OBJD)")
	(cd Objects;make "CC=$(CC)" "OPTG=$(DBG)" "OBJ=$(OBJD)")
	(cd GUI;make "CC=$(CC)" "OPTG=$(DBG)" "OBJ=$(OBJD)")
	(cd GLES;make -f OSX_Carbon.mk "CC=$(CC)" "OPTG=$(DBG)" "OBJ=$(OBJD)")
	(cd Null;make "CC=$(CC)" "OPTG=$(DBG)" "OBJ=$(OBJD)")
	make -f OSX_Carbon.mk "OBJ=$(OBJD)" libhwnull.a libhwgl.a libhwut.a


clean:
	rm */*.o
	rm */*/*.o

libhwgl.a: $(OBJ)Common.o $(OBJ)Objects.o $(OBJ)Gui.o $(OBJ)GL.o
	rm -f $@
	ar rv $@ $(OBJ)Common.o $(OBJ)Objects.o $(OBJ)Gui.o $(OBJ)GL.o $(OBJ)Null.o $(PNG)

libhwut.a: $(OBJ)Stubs.o $(OBJ)GL.o
	rm -f $@
	ar rv $@ $(OBJ)Stubs.o $(OBJ)GL.o $(PNG)

libhwnull.a: $(OBJ)Common.o $(OBJ)Objects.o $(OBJ)Gui.o $(OBJ)Null.o
	rm -f $@
	ar rv $@ $(OBJ)Common.o $(OBJ)Objects.o $(OBJ)Gui.o $(OBJ)Null.o $(PNG)

gfx2hw: Convert libhwnull.a
	(cd Convert;make)

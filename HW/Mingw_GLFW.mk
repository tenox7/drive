OBJ=Objs/
OBJD=Objs_d/
OPTG=-O 
DBG=-g 
GLFW=../../glfw-3.0
CC=gcc -fPIC -DWIN32 -DMINGW -DGLFW -I$(GLFW)/include
PNG=../LIBPNG/OBJ/readpng.o ../LIBPNG/OBJ/writepng.o
GLFW_O=../glfw-3.0/src/libglfw3.a
MAKE=mingw32-make

default:
	(cd Common;$(MAKE) "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	(cd Objects;$(MAKE) "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	(cd GUI;$(MAKE) "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	(cd GLES;$(MAKE) -f Linux_GLFW.mk "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	(cd Null;$(MAKE) "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	$(MAKE) -f Mingw_GLFW.mk "OBJ=$(OBJ)" libhwgl.a libhwnull.a libhwut.a

test:
	(cd Test;$(MAKE) "SHELL=/bin/bash" "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)" -f Linux_GLFW.mk)

all:	default test gfx2hw

all_debug:	debug y2k test gfx2hw

y2k:	
	(cd Test/y2k ;$(MAKE) "OPTG=$(DBG)" "OBJ=$(OBJ)")


gl:
	$(MAKE) default
	(cd GLES;$(MAKE) -f Linux_GLFW.mk "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	$(MAKE) "OBJ=$(OBJ)" libhwgl.a

debug:
	(cd Common;$(MAKE) "CC=$(CC)" "OPTG=$(DBG)" "OBJ=$(OBJD)")
	(cd Objects;$(MAKE) "CC=$(CC)" "OPTG=$(DBG)" "OBJ=$(OBJD)")
	(cd GUI;$(MAKE) "CC=$(CC)" "OPTG=$(DBG)" "OBJ=$(OBJD)")
	(cd GLES;$(MAKE) -f Linux_GLFW.mk "CC=$(CC)" "OPTG=$(DBG)" "OBJ=$(OBJD)")
	(cd Null;$(MAKE) "CC=$(CC)" "OPTG=$(DBG)" "OBJ=$(OBJD)")
	$(MAKE) -f Linux_GLFW.mk "OBJ=$(OBJD)" libhwnull.a libhwgl.a libhwut.a


clean:
	rm -f */*.o
	rm -f */*/*.o

libhwgl.a: $(OBJ)Common.o $(OBJ)Objects.o $(OBJ)Gui.o $(OBJ)GL.o $(OBJ)Null.o
	rm -f $@
	ar rv $@ $(OBJ)Common.o $(OBJ)Objects.o $(OBJ)Gui.o $(OBJ)GL.o $(OBJ)Null.o $(PNG) $(GLFW_O)

libhwut.a: $(OBJ)Stubs.o $(OBJ)GL.o $(OBJ)Null.o
	rm -f $@
	ar rv $@ $(OBJ)Stubs.o $(OBJ)GL.o $(OBJ)Null.o $(PNG) $(GLFW_O)

libhwnull.a: $(OBJ)Common.o $(OBJ)Objects.o $(OBJ)Gui.o $(OBJ)Null.o
	rm -f $@
	ar rv $@ $(OBJ)Common.o $(OBJ)Objects.o $(OBJ)Gui.o $(OBJ)Null.o $(PNG)

gfx2hw: Convert libhwnull.a
	(cd Convert;$(MAKE))

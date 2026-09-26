OBJ=Objs/
OBJD=Objs_d/
OPTG=-O2
DBG=-g
GLFW=$(shell brew --prefix glfw 2>/dev/null || echo /usr/local)
LEGACY=-fcommon -std=gnu89 -Wno-implicit-int -Wno-implicit-function-declaration \
	-Wno-return-type -Wno-int-conversion -Wno-incompatible-pointer-types \
	-Wno-deprecated-non-prototype -Wno-deprecated-declarations
CC=clang $(LEGACY) -DGLFW -I$(GLFW)/include
PNG=../LIBPNG/OBJ/readpng.o ../LIBPNG/OBJ/writepng.o
GLFW_O=

default:
	(cd Common;make "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	(cd Objects;make "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	(cd GUI;make "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	(cd GLES;make -f OSX_GLFW.mk "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	(cd Null;make "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	make -f OSX_GLFW.mk "OBJ=$(OBJ)" libhwgl.a libhwnull.a libhwut.a

test:
	(cd Test;make "SHELL=/bin/bash" "CC=$(CC)" "OPTG=$(OPTG)" "OBJ=$(OBJ)" -f OSX_GLFW.mk)

all:	default test gfx2hw

all_debug:	debug y2k test gfx2hw

y2k:	
	(cd Test/y2k ;make "OPTG=$(DBG)" "OBJ=$(OBJ)")

gl:
	make default
	(cd GLES;make -f OSX_GLFW.mk "OPTG=$(OPTG)" "OBJ=$(OBJ)")
	make "OBJ=$(OBJ)" libhwgl.a

debug:
	(cd Common;make "CC=$(CC)" "OPTG=$(DBG)" "OBJ=$(OBJD)")
	(cd Objects;make "CC=$(CC)" "OPTG=$(DBG)" "OBJ=$(OBJD)")
	(cd GUI;make "CC=$(CC)" "OPTG=$(DBG)" "OBJ=$(OBJD)")
	(cd GLES;make -f OSX_GLFW.mk "CC=$(CC)" "OPTG=$(DBG)" "OBJ=$(OBJD)")
	(cd Null;make "CC=$(CC)" "OPTG=$(DBG)" "OBJ=$(OBJD)")
	make -f OSX_GLFW.mk "OBJ=$(OBJD)" libhwnull.a libhwgl.a libhwut.a

clean:
	rm -f */Objs/*.o */Objs_d/*.o Objs/*.o Objs_d/*.o *.a

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
	(cd Convert;make)

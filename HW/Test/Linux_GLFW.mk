SHELL=/bin/bash

HWGL=../libhwgl.a
HWUT=../libhwut.a
BASELIBS=-L/usr/X11R6/lib -L/opt/graphics/OpenGL/lib -lpng -ljpeg -lglfw -lGL -lXext -lXrandr -lXi -lX11 -lrt -lpthread -ldl -lm
GLLIBS= $(HWGL) $(BASELIBS)
UTLIBS= $(HWUT) $(BASELIBS)

INC=../Inc/
OBJ=Objs/

CC=gcc -DGLFW
CFLAGS=-g -I$(INC)

TESTS=gauge_gl guitest_gl parse_gl poly_gl mesh_gl simple_gl sphere_gl text_gl \
	tm_gl welcome_gl vistest_gl hbgui_gl sphere_bump_gl

all:  $(TESTS)

clean:
	rm $(TESTS)

gauge_gl:	$(OBJ)gauge.o $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJ)gauge.o $(GLLIBS)

guitest_gl:	$(OBJ)guitest.o $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJ)guitest.o $(GLLIBS)

hbgui_gl:	$(OBJ)hbgui.o $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJ)hbgui.o $(GLLIBS)

mesh_gl:	$(OBJ)mesh.o $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJ)mesh.o $(GLLIBS)

parse_gl:	$(OBJ)parse.o $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJ)parse.o $(GLLIBS)

welcome_gl:	$(OBJ)welcome.o $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJ)welcome.o $(GLLIBS)

poly_gl:	$(OBJ)poly.o $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJ)poly.o $(GLLIBS)

simple_gl:	$(OBJ)simple.o $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJ)simple.o $(GLLIBS)

simple_ut:	$(OBJ)simple.o $(HWUT)
	$(CC) $(CFLAGS) -o $@ $(OBJ)simple.o $(UTLIBS)

sphere_gl:	$(OBJ)sphere.o $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJ)sphere.o $(GLLIBS)

sphere_bump_gl:	$(OBJ)sphere_bump.o $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJ)sphere_bump.o $(GLLIBS)

tm_gl:	$(OBJ)tm.o $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJ)tm.o $(GLLIBS)

tmtext_gl:	$(OBJ)tmtext.o $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJ)tmtext.o $(GLLIBS)

text_gl:	$(OBJ)text.o $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJ)text.o $(GLLIBS)

text_ut:	$(OBJ)text.o $(HWUT)
	$(CC) $(CFLAGS) -o $@ $(OBJ)text.o $(UTLIBS)

view_hw_gl:	$(OBJ)view_hw.o $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJ)view_hw.o $(GLLIBS)

vistest_gl:	$(OBJ)vistest.o $(HWGL)
	$(CC) $(CFLAGS) -o $@ $(OBJ)vistest.o $(GLLIBS)

$(OBJ)view_hw.o:	view_hw.c
	$(CC) $(CFLAGS) -c -o $@ view_hw.c

$(OBJ)gauge.o:	gauge.c
	$(CC) $(CFLAGS) -c -o $@ gauge.c

$(OBJ)guitest.o:	guitest.c
	$(CC) $(CFLAGS) -c -o $@ guitest.c

$(OBJ)hbgui.o:	hbgui.c
	$(CC) $(CFLAGS) -c -o $@ hbgui.c

$(OBJ)mesh.o:	mesh.c
	$(CC) $(CFLAGS) -c -o $@ mesh.c

$(OBJ)parse.o:	parse.c
	$(CC) $(CFLAGS) -c -o $@ parse.c

$(OBJ)welcome.o:	welcome.c
	$(CC) $(CFLAGS) -c -o $@ welcome.c

$(OBJ)poly.o:	poly.c
	$(CC) $(CFLAGS) -c -o $@ poly.c

$(OBJ)simple.o:	simple.c
	$(CC) $(CFLAGS) -c -o $@ simple.c

$(OBJ)sphere.o:	sphere.c
	$(CC) $(CFLAGS) -c -o $@ sphere.c

$(OBJ)sphere_bump.o:	sphere_bump.c
	$(CC) $(CFLAGS) -c -o $@ sphere_bump.c

$(OBJ)tm.o:	tm.c
	$(CC) $(CFLAGS) -c -o $@ tm.c

$(OBJ)tmtext.o:	tmtext.c
	$(CC) $(CFLAGS) -c -o $@ tmtext.c

$(OBJ)text.o:	text.c
	$(CC) $(CFLAGS) -c -o $@ text.c

$(OBJ)vistest.o:	vistest.c
	$(CC) $(CFLAGS) -c -o $@ vistest.c

$(OBJ)write.o:	write.c
	$(CC) $(CFLAGS) -c -o $@ write.c

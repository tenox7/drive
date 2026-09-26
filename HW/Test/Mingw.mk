##
##  Makefile for Windows NT
##


CC=x86_64-w64-mingw32-gcc
#OPTG    = -g
OPTG    = -O
CFLAGS	= -I../Inc -DWIN32 -DWIN_GL $(OPTG)
GUILIBS = -ljpeg -lpng -lwinmm -lkernel32 -ladvapi32 -luser32 -lgdi32 -lcomdlg32
LIBS	= ../libhwgl.a -lglu32 -lopengl32 $(GUILIBS)
UTLIBS	= ../libhwut.a -lglu32 -lopengl32 $(GUILIBS)
EXES	= mesh.exe parse.exe poly.exe print.exe simple.exe sphere.exe \
		text.exe tm.exe welcome.exe write.exe vistest.exe tmtext.exe \
                gauge.exe hbgui.exe sphere_bump.exe
LOPTS	=

all		: $(EXES)

gauge.exe	:	gauge.o ../libhwgl.a
	$(CC) $(LOPTS) -o $@ gauge.o $(LIBS)

hbgui.exe	:	hbgui.o ../libhwgl.a
	$(CC) $(LOPTS) -o $@ hbgui.o $(LIBS)

mesh.exe	:	mesh.o ../libhwgl.a
	$(CC) $(LOPTS) -o $@ mesh.o $(LIBS)

parse.exe	:	parse.o ../libhwgl.a
	$(CC) $(LOPTS) -o $@ parse.o $(LIBS)

poly.exe	:	poly.o ../libhwgl.a
	$(CC) $(LOPTS) -o $@ poly.o $(LIBS)

print.exe	:	print.o ../libhwgl.a
	$(CC) $(LOPTS) -o $@ print.o $(LIBS)

simple.exe	:	simple.o ../libhwgl.a
	$(CC) $(LOPTS) -o $@ simple.o $(LIBS)

sphere.exe	:	sphere.o ../libhwgl.a
	$(CC) $(LOPTS) -o $@ sphere.o $(LIBS)

sphere_bump.exe	:	sphere_bump.o ../libhwgl.a
	$(CC) $(LOPTS) -o $@ sphere_bump.o $(LIBS)

text.exe	:	text.o ../libhwgl.a
	$(CC) $(LOPTS) -o $@ text.o $(LIBS)

text_ut.exe	:	text.o ../libhwut.a
	$(CC) $(LOPTS) -o $@ text.o $(UTLIBS)

tm.exe	:	tm.o ../libhwgl.a
	$(CC) $(LOPTS) -o $@ tm.o $(LIBS)

tmtext.exe	:	tmtext.o ../libhwgl.a
	$(CC) $(LOPTS) -o $@ tmtext.o $(LIBS)

view_hw.exe	:	view_hw.o ../libhwgl.a
	$(CC) $(LOPTS) -o $@ view_hw.o $(LIBS)

welcome.exe	:	welcome.o ../libhwgl.a
	$(CC) $(LOPTS) -o $@ welcome.o $(LIBS)

write.exe	:	write.o ../libhwgl.a
	$(CC) $(LOPTS) -o $@ write.o $(LIBS)

vistest.exe	:	vistest.o ../libhwgl.a
	$(CC) $(LOPTS) -o $@ vistest.o $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	del *.o *.pdb *.ilk *.ncb *~

clobber: clean
	del *.exe

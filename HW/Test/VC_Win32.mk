##
##  Makefile for Windows NT
##


!include <ntwin32.mak>


#OPTG    = -Zi
OPTG    = -O2
CFLAGS	= -I../Inc -DWIN32 -DWIN_GL -nologo -W1 $(OPTG)
LIBS	= ../HW.lib ../../JPEG/libjpeg.lib glu32.lib opengl32.lib $(guilibs)
UTLIBS	= ../HWUT.lib ../../JPEG/libjpeg.lib glu32.lib opengl32.lib $(guilibs)
EXES	= mesh.exe parse.exe poly.exe print.exe simple.exe sphere.exe \
		text.exe tm.exe welcome.exe write.exe vistest.exe tmtext.exe \
                gauge.exe hbgui.exe sphere_bump.exe
#TBD: view_hw.exe
#LOPTS	= -nologo -nodefaultlib:libcd /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup
#LOPTS	= -nologo -nodefaultlib:libcd /debug
LOPTS	= -nologo -nodefaultlib:LIBCMTD

all		: $(EXES)

gauge.exe	:	gauge.obj ../HW.lib
	$(link) $(LOPTS) -out:$@ gauge.obj $(LIBS)

hbgui.exe	:	hbgui.obj ../HW.lib
	$(link) $(LOPTS) -out:$@ hbgui.obj $(LIBS)

mesh.exe	:	mesh.obj ../HW.lib
	$(link) $(LOPTS) -out:$@ mesh.obj $(LIBS)

parse.exe	:	parse.obj ../HW.lib
	$(link) $(LOPTS) -out:$@ parse.obj $(LIBS)

poly.exe	:	poly.obj ../HW.lib
	$(link) $(LOPTS) -out:$@ poly.obj $(LIBS)

print.exe	:	print.obj ../HW.lib
	$(link) $(LOPTS) -out:$@ print.obj $(LIBS)

simple.exe	:	simple.obj ../HW.lib
	$(link) $(LOPTS) -out:$@ simple.obj $(LIBS)

sphere.exe	:	sphere.obj ../HW.lib
	$(link) $(LOPTS) -out:$@ sphere.obj $(LIBS)

sphere_bump.exe	:	sphere_bump.obj ../HW.lib
	$(link) $(LOPTS) -out:$@ sphere_bump.obj $(LIBS)

text.exe	:	text.obj ../HW.lib
	$(link) $(LOPTS) -out:$@ text.obj $(LIBS)

text_ut.exe	:	text.obj ../HWUT.lib
	$(link) $(LOPTS) -out:$@ text.obj $(UTLIBS)

tm.exe	:	tm.obj ../HW.lib
	$(link) $(LOPTS) -out:$@ tm.obj $(LIBS)

tmtext.exe	:	tmtext.obj ../HW.lib
	$(link) $(LOPTS) -out:$@ tmtext.obj $(LIBS)

view_hw.exe	:	view_hw.obj ../HW.lib
	$(link) $(LOPTS) -out:$@ view_hw.obj $(LIBS)

welcome.exe	:	welcome.obj ../HW.lib
	$(link) $(LOPTS) -out:$@ welcome.obj $(LIBS)

write.exe	:	write.obj ../HW.lib
	$(link) $(LOPTS) -out:$@ write.obj $(LIBS)

vistest.exe	:	vistest.obj ../HW.lib
	$(link) $(LOPTS) -out:$@ vistest.obj $(LIBS)

.c.obj:
	$(CC) $(CFLAGS) -c $<

clean:
	del *.obj *.pdb *.ilk *.ncb *~

clobber: clean
	del *.exe

##
##  Makefile for Windows NT
##


!include <ntwin32.mak>


#OPTG    = -Zi
OPTG    = -O2
CFLAGS	= -I../Inc -DWIN32 -nologo -W1 $(OPTG)
DX9="$(DX9SDK)\lib\x86\d3d9.lib" "$(DX9SDK)\lib\x86\d3dx9.lib"
LIBS	= ../hwdx.lib ../../JPEG/libjpeg.lib $(DX9) $(guilibs)
EXES	= dxmesh.exe dxparse.exe dxpoly.exe dxprint.exe dxsimple.exe \
		dxsphere.exe dxtext.exe dxtm.exe dxwelcome.exe dxwrite.exe \
		dxtmtext.exe
#TBD: view_hw.exe
#LOPTS	= -nologo -nodefaultlib:libcd /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup
LOPTS	= -nologo -nodefaultlib:libcd /debug

all		: $(EXES)

dxmesh.exe	:	mesh.obj ../hwdx.lib
	$(link) $(LOPTS) -out:$@ mesh.obj $(LIBS)

dxparse.exe	:	parse.obj ../hwdx.lib
	$(link) $(LOPTS) -out:$@ parse.obj $(LIBS)

dxpoly.exe	:	poly.obj ../hwdx.lib
	$(link) $(LOPTS) -out:$@ poly.obj $(LIBS)

dxprint.exe	:	print.obj ../hwdx.lib
	$(link) $(LOPTS) -out:$@ print.obj $(LIBS)

dxsimple.exe	:	simple.obj ../hwdx.lib
	$(link) $(LOPTS) -out:$@ simple.obj $(LIBS)

dxsphere.exe	:	sphere.obj ../hwdx.lib
	$(link) $(LOPTS) -out:$@ sphere.obj $(LIBS)

dxsphere_bump.exe	:	sphere_bump.obj ../hwdx.lib
	$(link) $(LOPTS) -out:$@ sphere_bump.obj $(LIBS)

dxtext.exe	:	text.obj ../hwdx.lib
	$(link) $(LOPTS) -out:$@ text.obj $(LIBS)

dxtm.exe	:	tm.obj ../hwdx.lib
	$(link) $(LOPTS) -out:$@ tm.obj $(LIBS)

dxtmtext.exe	:	tmtext.obj ../hwdx.lib
	$(link) $(LOPTS) -out:$@ tmtext.obj $(LIBS)

dxview_hw.exe	:	view_hw.obj ../hwdx.lib
	$(link) $(LOPTS) -out:$@ view_hw.obj $(LIBS)

dxwelcome.exe	:	welcome.obj ../hwdx.lib
	$(link) $(LOPTS) -out:$@ welcome.obj $(LIBS)

dxwrite.exe	:	write.obj ../hwdx.lib
	$(link) $(LOPTS) -out:$@ write.obj $(LIBS)

.c.obj:
	$(CC) $(CFLAGS) -c $<

clean:
	del *.obj *.pdb *.ilk *.ncb *~

clobber: clean
	del *.exe

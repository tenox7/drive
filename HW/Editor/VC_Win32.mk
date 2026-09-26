##
##  Makefile for Windows NT
##


!include <win32.mak>


CFLAGS   = -I../Inc -DWIN32 -nologo -W1 -O2
LIBS     = ../HW.lib ../../JPEG/libjpeg.lib glu32.lib opengl32.lib wsock32.lib $(guilibs)
EXES     = hwedit.exe cons.exe
#LOPTS	= -nologo -nodefaultlib:libcd /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup
LOPTS	= -nologo -nodefaultlib:libcd

OBJS=camera.obj command.obj ipc.obj main.obj prop.obj streams.obj

all             : $(EXES)

hwedit.exe	:	$(OBJS)
	$(link) $(LOPTS) -out:$@ $(OBJS) $(LIBS)

cons.exe        :       cons.obj ipc.obj
	$(link) $(LOPTS) -out:$@ cons.obj ipc.obj $(LIBS)

.c.obj:
	$(CC) $(CFLAGS) -c $<

clean:
	del *.obj *.pdb *.ilk *.ncb *~

clobber: clean
	del *.exe

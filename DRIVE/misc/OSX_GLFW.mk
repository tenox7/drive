#DEBUG=
PDF_DEBUG=-g
#OPTG=-O2
OPTG=-g
CFLAGS=$(OPTG) -DGLFW -DMAC -I../libnum -I../libphysics -I../include -I../../HW/Inc -I../libprims -I../../pdcurses -I../server/objects

all:    drive2pdf

OBJS=drive2pdf.o pdf.o

SRV_LIBS=../server.a  \
	../libnum.a \
	../libphysics.a \
	../libprims.a \
	../drive_physics.a \
	../objects.a \
	../shared.a

IMPORTS=../../HW/libhwnull.a ../../JPEG/libjpeg.a
SYSLIBS=-lcurses

drive2pdf:  $(OBJS) $(SRV_LIBS) $(IMPORTS)
	(cd ..; make -f OSX_GLFW.mk drive_server_rule)
	$(CC) -o drive2pdf \
		$(PDF_DEBUG) \
		$(OBJS) \
		$(SRV_LIBS) \
		$(IMPORTS) \
		$(SYSLIBS)

clean:
	rm -f *.o drive2pdf

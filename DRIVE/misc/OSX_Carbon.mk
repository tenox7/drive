#DEBUG=
PDF_DEBUG=-g
#OPTG=-O2
OPTG=-g
CFLAGS=$(OPTG) -m32 -DMAC -I../libnum -I../libphysics -I../include -I../../HW/Inc -I../libprims -I../../pdcurses -I../server/objects

all:    drive2pdf

OBJS=drive2pdf.o pdf.o

SRV_LIBS=../server.a  \
	../libnum.a \
	../libphysics.a \
	../libprims.a \
	../drive_physics.a \
	../objects.a \
	../shared.a

IMPORTS=../../HW/libhwnull.a ../../JPEG/libjpeg32.a
SYSLIBS=-lcurses

drive2pdf:  $(OBJS) $(SRV_LIBS) $(IMPORTS)
	(cd ..; make -f OSX_Carbon.mk drive_server_rule)
	$(CC) -m32 -o drive2pdf \
		$(PDF_DEBUG) \
		$(OBJS) \
		$(SRV_LIBS) \
		$(IMPORTS) \
		$(SYSLIBS)

clean:
	rm -f *.o drive2pdf

OBJS=camera.o \
	clook.o \
	daylight.o \
	disp_ctrl.o \
	drive_msg.o \
	drive_sb.o \
	explosion.o \
	frame.o \
	headlight.o \
	joystick.o \
	parse_rcfile.o \
	sound.o \
	stars.o

OBJ1=$(OBJS) drive.o

LIBS=../client.a

#OPTG=-O2
OPTG=-g
CFLAGS=$(OPTG) -m32 -DMAC -I../server/objects -I../libnum -I../libphysics -I../include -I../../HW/Inc -I../libprims

all:	$(LIBS)

clean:
	rm -f *.o $(LIBS)

../client.a:     $(OBJ1)
	rm -f $@
	ar rv $@ $(OBJS)

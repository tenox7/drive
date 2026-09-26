#SRV_DEBUG=
SRV_DEBUG=-g
#CLI_DEBUG=
CLI_DEBUG=-g


all:	drive_server_rule drive_rule

drive_server_rule:
	(cd libnum; make -f OSX_Carbon.mk)
	(cd libphysics; make -f OSX_Carbon.mk)
	(cd libprims; make -f OSX_Carbon.mk)
	(cd server/drive_physics; make -f OSX_Carbon.mk)
	(cd server/objects; make -f OSX_Carbon.mk)
	(cd server; make -f OSX_Carbon.mk)
	(cd shared; make -f OSX_Carbon.mk)
	make -f OSX_Carbon.mk drive_server

SRV_LIBS=server.a \
	libnum.a \
	libphysics.a \
	libprims.a \
	drive_physics.a \
	objects.a \
	shared.a \
	../HW/libhwnull.a \
	../JPEG/libjpeg32.a

drive_server: server/drive_server.o $(SRV_LIBS)
	$(CC) -m32 -o drive_server \
		$(SRV_DEBUG) \
		server/drive_server.o \
		$(SRV_LIBS) \
		-lcurses

drive_rule:
	(cd client; make -f OSX_Carbon.mk)
	make -f OSX_Carbon.mk drive

CLI_LIBS=client.a \
	shared.a \
	../HW/libhwgl.a \
	../JPEG/libjpeg32.a

drive:  client/drive.o $(CLI_LIBS)
	$(CC) -m32 -o drive \
		$(CLI_DEBUG) \
		client/drive.o \
		$(CLI_LIBS) \
		-framework Opengl -framework AGL -framework Carbon

clean:
	(cd libnum; make -f OSX_Carbon.mk clean)
	(cd libphysics; make -f OSX_Carbon.mk clean)
	(cd libprims; make -f OSX_Carbon.mk clean)
	(cd server/drive_physics; make -f OSX_Carbon.mk clean)
	(cd server/objects; make -f OSX_Carbon.mk clean)
	(cd server; make -f OSX_Carbon.mk clean)
	(cd shared; make -f OSX_Carbon.mk clean)
	(cd client; make -f OSX_Carbon.mk clean)

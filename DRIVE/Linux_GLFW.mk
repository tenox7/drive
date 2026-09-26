# DRIVE for Linux with GLFW.  Note: the sources spell "not the X11/Motif
# build" as WIN32 || MAC, so the subdir makefiles define MAC here too.
SRV_DEBUG=-g
CLI_DEBUG=-g

# Overridable from the environment, which is how package.linux.sh links
# GLFW and curses statically for a tarball that runs on any distribution.
GLFW_LIB ?= -lglfw
CURSES_LIB ?= -lncurses

all:	drive_server_rule drive_rule

drive_server_rule:
	(cd libnum; make -f Linux_GLFW.mk)
	(cd libphysics; make -f Linux_GLFW.mk)
	(cd libprims; make -f Linux_GLFW.mk)
	(cd server/drive_physics; make -f Linux_GLFW.mk)
	(cd server/objects; make -f Linux_GLFW.mk)
	(cd server; make -f Linux_GLFW.mk)
	(cd shared; make -f Linux_GLFW.mk)
	make -f Linux_GLFW.mk drive_server

SRV_LIBS=server.a \
	libnum.a \
	libphysics.a \
	libprims.a \
	drive_physics.a \
	objects.a \
	shared.a \
	../HW/libhwnull.a \
	../JPEG/libjpeg.a

drive_server: server/drive_server.o $(SRV_LIBS)
	$(CC) -o drive_server \
		$(SRV_DEBUG) \
		server/drive_server.o \
		-Wl,--start-group $(SRV_LIBS) -Wl,--end-group \
		$(CURSES_LIB) -lm -lpthread

drive_rule:
	(cd client; make -f Linux_GLFW.mk)
	make -f Linux_GLFW.mk drive

CLI_LIBS=client.a \
	shared.a \
	../HW/libhwgl.a \
	../JPEG/libjpeg.a

drive:  client/drive.o $(CLI_LIBS)
	$(CC) -o drive \
		$(CLI_DEBUG) \
		client/drive.o \
		-Wl,--start-group $(CLI_LIBS) -Wl,--end-group \
		$(GLFW_LIB) -lGL -lm -ldl -lpthread

clean:
	(cd libnum; make -f Linux_GLFW.mk clean)
	(cd libphysics; make -f Linux_GLFW.mk clean)
	(cd libprims; make -f Linux_GLFW.mk clean)
	(cd server/drive_physics; make -f Linux_GLFW.mk clean)
	(cd server/objects; make -f Linux_GLFW.mk clean)
	(cd server; make -f Linux_GLFW.mk clean)
	(cd shared; make -f Linux_GLFW.mk clean)
	(cd client; make -f Linux_GLFW.mk clean)
	rm -f drive drive_server *.a

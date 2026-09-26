#SRV_DEBUG=
SRV_DEBUG=-g
GLFW=$(shell brew --prefix glfw 2>/dev/null || echo /usr/local)
# Link GLFW statically when the archive is there, so the binary carries no
# dylib of its own and the .app needs nothing installed.
GLFW_LIB=$(shell test -f $(GLFW)/lib/libglfw3.a \
	&& echo $(GLFW)/lib/libglfw3.a || echo -L$(GLFW)/lib -lglfw)
#CLI_DEBUG=
CLI_DEBUG=-g


all:	drive_server_rule drive_rule

drive_server_rule:
	(cd libnum; make -f OSX_GLFW.mk)
	(cd libphysics; make -f OSX_GLFW.mk)
	(cd libprims; make -f OSX_GLFW.mk)
	(cd server/drive_physics; make -f OSX_GLFW.mk)
	(cd server/objects; make -f OSX_GLFW.mk)
	(cd server; make -f OSX_GLFW.mk)
	(cd shared; make -f OSX_GLFW.mk)
	make -f OSX_GLFW.mk drive_server

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
		$(SRV_LIBS) \
		-lcurses

drive_rule:
	(cd client; make -f OSX_GLFW.mk)
	make -f OSX_GLFW.mk drive

CLI_LIBS=client.a \
	shared.a \
	../HW/libhwgl.a \
	../JPEG/libjpeg.a

drive:  client/drive.o $(CLI_LIBS)
	$(CC) -o drive \
		$(CLI_DEBUG) \
		client/drive.o \
		$(CLI_LIBS) \
		$(GLFW_LIB) \
		-framework OpenGL -framework Cocoa -framework IOKit \
		-framework CoreFoundation -framework CoreVideo \
		-framework QuartzCore -lm

clean:
	(cd libnum; make -f OSX_GLFW.mk clean)
	(cd libphysics; make -f OSX_GLFW.mk clean)
	(cd libprims; make -f OSX_GLFW.mk clean)
	(cd server/drive_physics; make -f OSX_GLFW.mk clean)
	(cd server/objects; make -f OSX_GLFW.mk clean)
	(cd server; make -f OSX_GLFW.mk clean)
	(cd shared; make -f OSX_GLFW.mk clean)
	(cd client; make -f OSX_GLFW.mk clean)

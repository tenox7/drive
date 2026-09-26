# DRIVE for Windows with GLFW, built under MSYS2/MinGW-w64.  GLFW and the
# runtime are linked statically so the two .exe files stand on their own.
CC=gcc
SRV_DEBUG=-g
CLI_DEBUG=-g

all:	drive_server_rule drive_rule

drive_server_rule:
	(cd libnum; make -f Mingw_GLFW.mk)
	(cd libphysics; make -f Mingw_GLFW.mk)
	(cd libprims; make -f Mingw_GLFW.mk)
	(cd server/drive_physics; make -f Mingw_GLFW.mk)
	(cd server/objects; make -f Mingw_GLFW.mk)
	(cd server; make -f Mingw_GLFW.mk)
	(cd shared; make -f Mingw_GLFW.mk)
	make -f Mingw_GLFW.mk drive_server.exe

SRV_LIBS=server.a \
	libnum.a \
	libphysics.a \
	libprims.a \
	drive_physics.a \
	objects.a \
	shared.a \
	../HW/libhwnull.a \
	../JPEG/libjpeg.a \
	../PDCURSES/wincon/pdcurses.a

drive_server.exe: server/drive_server.o $(SRV_LIBS)
	$(CC) -o drive_server.exe \
		$(SRV_DEBUG) \
		server/drive_server.o \
		-Wl,--start-group $(SRV_LIBS) -Wl,--end-group \
		-static -lws2_32 -luser32 -lwinmm -lm

drive_rule:
	(cd client; make -f Mingw_GLFW.mk)
	make -f Mingw_GLFW.mk drive.exe

CLI_LIBS=client.a \
	shared.a \
	../HW/libhwgl.a \
	../JPEG/libjpeg.a

# -mwindows: no stray console window when the game is started from Explorer.
drive.exe:  client/drive.o $(CLI_LIBS)
	$(CC) -mwindows -o drive.exe \
		$(CLI_DEBUG) \
		client/drive.o \
		-Wl,--start-group $(CLI_LIBS) -Wl,--end-group \
		-static -lglfw3 -lopengl32 -lgdi32 -lws2_32 -luser32 -lwinmm -lm

clean:
	(cd libnum; make -f Mingw_GLFW.mk clean)
	(cd libphysics; make -f Mingw_GLFW.mk clean)
	(cd libprims; make -f Mingw_GLFW.mk clean)
	(cd server/drive_physics; make -f Mingw_GLFW.mk clean)
	(cd server/objects; make -f Mingw_GLFW.mk clean)
	(cd server; make -f Mingw_GLFW.mk clean)
	(cd shared; make -f Mingw_GLFW.mk clean)
	(cd client; make -f Mingw_GLFW.mk clean)
	rm -f drive.exe drive_server.exe *.a

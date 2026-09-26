OBJ=Objs/
INC=../Inc/

HDRS=$(INC)hw.h $(INC)hw_os.h $(INC)hw_gl.h $(INC)hw_display.h

OBJS=$(OBJ)gl_graph.o $(OBJ)gl_visual.o $(OBJ)glfw_visual.o $(OBJ)gl_program.o \
	$(OBJ)gl_camera.o $(OBJ)gl_list.o $(OBJ)gl_state.o \
	$(OBJ)glues_mipmap.o $(OBJ)glues_project.o

OPTG=-O
CC=c89
CFLAGS=$(OPTG) -I/usr/include/X11 -I/opt/graphics/OpenGL/include -I$(INC)

all:	../$(OBJ)GL.o

../$(OBJ)GL.o:	$(OBJS)
	ld -r $(OBJS) -o ../$(OBJ)GL.o

$(OBJ)gl_camera.o:	gl_camera.c $(HDRS)
	$(CC) $(CFLAGS) gl_camera.c -c -o $@

$(OBJ)gl_graph.o:	gl_graph.c $(HDRS)
	$(CC) $(CFLAGS) gl_graph.c -c -o $@

$(OBJ)gl_list.o:	gl_list.c $(HDRS)
	$(CC) $(CFLAGS) gl_list.c -c -o $@

$(OBJ)gl_program.o:	gl_program.c $(HDRS)
	$(CC) $(CFLAGS) gl_program.c -c -o $@

$(OBJ)gl_state.o:	gl_state.c $(HDRS)
	$(CC) $(CFLAGS) gl_state.c -c -o $@

$(OBJ)gl_visual.o:	gl_visual.c $(HDRS)
	$(CC) $(CFLAGS) gl_visual.c -c -o $@

$(OBJ)glfw_visual.o:	glfw_visual.c $(HDRS)
	$(CC) $(CFLAGS) glfw_visual.c -c -o $@

$(OBJ)glues_mipmap.o:   glues_mipmap.c $(HDRS)
	$(CC) $(CFLAGS) glues_mipmap.c -c -o $@

$(OBJ)glues_project.o:  glues_project.c $(HDRS)
	$(CC) $(CFLAGS) glues_project.c -c -o $@

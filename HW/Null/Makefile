OBJ=Objs/
INC=../Inc/

HDRS=$(INC)hw.h $(INC)hw_os.h $(INC)hw_display.h

OBJS=$(OBJ)null.o

OPTG=-O +Olibcalls
CC=c89 -D_HPUX_SOURCE
CFLAGS=$(OPTG) -I$(INC)

all:	../$(OBJ)Null.o

../$(OBJ)Null.o:	$(OBJS)
	ld -r $(OBJS) -o ../$(OBJ)Null.o

$(OBJ)null.o:	null.c $(HDRS)
	$(CC) $(CFLAGS) null.c -c -o $@

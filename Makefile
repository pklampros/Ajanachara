# 2005-05-05 by gf

# generic compiler and linker settings:
CC     = g++
CFLAGS = -O -Wall #-g
INCDIR = -I../veLib/src/include -I../veLib/external/include -I../fltk -I/usr/include/SDL
LIB    = -lve -lfltk_images -lfltk_gl -lfltk -lpng -lz -ljpeg -lSDLmain -lSDL
LINLIB = -lGL -lGLU -lXext -lXi -lX11 -lm -lpthread
WINLIB = -lopengl32 -lglu32 -luser32 -lgdi32 -lkernel32 -luser32 -lole32 -luuid -lcomctl32 -lwsock32 -lsupc++ -mwindows -s
LIBDIR = -L../fltk/lib -L../veLib/src/lib -L../veLib/external/lib

# generic platform specific rules:
ARCH            = $(shell uname -s)
ifeq ($(ARCH),Linux)
  LIBS          = $(LIBDIR) -L/usr/X11R6/lib $(LIB) $(LINLIB)
  EXESUFFIX     =
else  # windows, MinGW
  LIBS          = $(LIBDIR) -lmingw32 $(LIB) $(WINLIB)
  EXESUFFIX     = .exe
endif

# project specific targets:
SRC = ajanachara.cpp visiGraph.cpp flAux.cpp veIoObj.cpp glMaterial.cpp gl2ps.c
HDR = visiGraph.h flAux.h veIoObj.h glMaterial.h gl2ps.h 
OBJ = ajanachara.o visiGraph.o flAux.o veIoObj.o glMaterial.o gl2ps.o
EXE = ajanachara$(EXESUFFIX)

# project specific build rules:
$(EXE) : $(OBJ) ../veLib/src/lib/libve.a
	$(CC) $(CFLAGS) $(OBJ) $(LIBS) -o $@

all: $(EXE) anavis

anavis$(EXESUFFIX): anavis.o visiGraph.o
	$(CC) $(CFLAGS) $(LIBDIR) $< visiGraph.o $(LIBS) -o $@

flAux.o: flAux.cpp flAux.h
	$(CC) $(CFLAGS) $(INCDIR) -c $<

$(EXE).o: $(EXE).cpp $(HDR)
visiGraph.o: visiGraph.cpp visiGraph.h
viIoObj.o: veIoObj.cpp veIoObj.h glMaterial.h
glMaterial.o: glMaterial.cpp glMaterial.h
gl2ps.o:        gl2ps.c gl2ps.h


# project specific dependencies:
vermelho.o: vermelho.cpp $(HDR)

# generic rules and targets:
.cpp.o:
	$(CC) $(CFLAGS) $(INCDIR) -c $<

.c.o:
	$(CC) $(CFLAGS) $(INCDIR) -c $<

clean:
	rm -f $(OBJ) $(EXE) *~


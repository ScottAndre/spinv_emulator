CC=gcc
CFLAGS=-g -Wall $(GTKFLAGS)
GTKFLAGS=`pkg-config --cflags gtk+-3.0`
LIBS=-lpthread $(GTKLIBS)
GTKLIBS=`pkg-config --libs gtk+-3.0`

ODIR=obj

OCOMPILE=$(CC) $(CFLAGS) -o $@ -c

spinv_emulator : $(ODIR)/emulator.o $(ODIR)/cpu8080.o $(ODIR)/display.o $(ODIR)/interrupts.o $(ODIR)/ports.o $(ODIR)/controls.o $(ODIR)/disassembler8080.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(ODIR)/emulator.o : emulator.c emulator.h cpu8080.h interrupts.h controls.h display.h ports.h
	$(OCOMPILE) emulator.c

$(ODIR)/cpu8080.o : cpu8080.c cpu8080.h disassembler8080.h ports.h interrupts.h
	$(OCOMPILE) cpu8080.c
	#$(OCOMPILE) -D CPU_PRINT cpu8080.c

$(ODIR)/display.o : display.c display.h controls.h emulator.h
	$(OCOMPILE) display.c

$(ODIR)/interrupts.o : interrupts.c interrupts.h
	$(OCOMPILE) interrupts.c

$(ODIR)/ports.o : ports.c ports.h controls.h
	$(OCOMPILE) ports.c

$(ODIR)/controls.o : controls.c controls.h
	$(OCOMPILE) controls.c

$(ODIR)/disassembler8080.o : disassembler8080.c disassembler8080.h
	$(OCOMPILE) disassembler8080.c

# here, the in-line pkg-config commands generate the necessary compiler flags and library links to use the gtk+-3.0 library
gtk-example : misc/gtk-example.c misc/gtk-draw-example.c
	$(CC) $(GTKFLAGS) -o misc/gtk-example misc/gtk-example.c $(GTKLIBS)
	$(CC) $(GTKFLAGS) -o misc/gtk-draw-example misc/gtk-draw-example.c $(GTKLIBS)

.PHONY : clean setup

setup :
	mkdir $(ODIR)

clean :
	rm -f $(ODIR)/*.o

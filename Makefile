CC=gcc
CFLAGS=-g -Wall

ODIR=obj

OCOMPILE=$(CC) $(CFLAGS) -o $@ -c

emulator8080 : $(ODIR)/cpu8080.o $(ODIR)/disassembler8080.o $(ODIR)/emulator8080.o $(ODIR)/ports.o
	$(CC) $(CFLAGS) -o $@ $^

$(ODIR)/cpu8080.o : cpu8080.c cpu8080.h disassembler8080.h ports.h
	$(OCOMPILE) cpu8080.c

$(ODIR)/ports.o : ports.c ports.h
	$(OCOMPILE) ports.c

$(ODIR)/disassembler8080.o : disassembler8080.c disassembler8080.h
	$(OCOMPILE) disassembler8080.c

$(ODIR)/emulator8080.o : emulator8080.c cpu8080.h
	$(OCOMPILE) emulator8080.c

# here, the in-line pkg-config commands generate the necessary compiler flags and library links to use the gtk+-3.0 library
gtk-example : misc/gtk-example.c	
	$(CC) `pkg-config --cflags gtk+-3.0` -o misc/$@ $^ `pkg-config --libs gtk+-3.0`

.PHONY : clean setup

setup :
	mkdir $(ODIR)

clean :
	rm -f $(ODIR)/*.o

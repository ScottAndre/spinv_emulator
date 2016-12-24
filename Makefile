CC=gcc
CFLAGS=-g -Wall

ODIR=obj

OCOMPILE=$(CC) $(CFLAGS) -o $@ -c

emulator8080 : $(ODIR)/cpu8080.o $(ODIR)/disassembler8080.o $(ODIR)/emulator8080.o
	$(CC) $(CFLAGS) -o $@ $^

$(ODIR)/cpu8080.o : cpu8080.c cpu8080.h disassembler8080.h
	$(OCOMPILE) cpu8080.c

$(ODIR)/disassembler8080.o : disassembler8080.c disassembler8080.h
	$(OCOMPILE) disassembler8080.c

$(ODIR)/emulator8080.o : emulator8080.c cpu8080.h
	$(OCOMPILE) emulator8080.c

.PHONY : clean setup

setup :
	mkdir $(ODIR)

clean :
	rm -f $(ODIR)/*.o

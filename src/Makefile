INCDIR =../include
CC=gcc
CFLAGS=-Wall -g `pkg-config --cflags --libs gtk+-2.0 r_core` -I$(INCDIR)

OBJDIR=obj
LIBDIR =../lib

_DEPS = disas.h hypertext.h disas_view.h util.h

DEPS = $(patsubst %,$(INCDIR)/%,$(_DEPS))

_OBJ = disas.o hypertext.o  disas_view.o  util.o
OBJ = $(patsubst %,$(OBJDIR)/%,$(_OBJ))


$(OBJDIR)/%.o: $(LIBDIR)/%.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS)

r2gtk: $(OBJ)
		$(CC) -o $@ $^  r2gtk.c $(CFLAGS)

.PHONY: clean

clean:
		rm -f $(OBJDIR)/*.o *~ core $(INCDIR)/*~ $(LIBDIR)/*~ 

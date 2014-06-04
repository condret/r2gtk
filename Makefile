EXES = disas_view
CC = cc
CFLAGS = -Wall -g `pkg-config --cflags --libs gtk+-2.0 r_core`
GMODULE = `pkg-config --cflags --libs gmodule-2.0`

all: 
	$(MAKE) $(EXES);
	
%: %.c
	$(CC) $(CFLAGS) $@.c -o $@

clean:
	rm -f $(EXES)

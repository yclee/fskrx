all: fskrx

LDFLAGS+=-lspandsp -ltiff

clean:
	-rm -f *.o

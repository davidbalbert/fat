OBJS = \
			 fat.o\
			 mbr.o\
			 main.o

LDFLAGS=-lc

readfat: $(OBJS)
	$(LD) $(LDFLAGS) -o readfat $(OBJS)

clean:
	rm -f *.o

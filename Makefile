OBJS = \
			 fat.o\
			 mbr.o\
			 main.o

CFLAGS = -m64 -O0 -MD -Wall -Werror -g
LDFLAGS = -lc

fat: $(OBJS)
	$(LD) $(LDFLAGS) -o fat $(OBJS)

-include *.d

.PHONY: clean
clean:
	rm -f *.o *.d fat

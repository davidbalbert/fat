OBJS = \
			 fat.o\
			 mbr.o\
			 main.o

CFLAGS = -m64 -O0 -MD -Wall -Werror -g
LDFLAGS = -lc

readfat: $(OBJS)
	$(LD) $(LDFLAGS) -o readfat $(OBJS)

-include *.d

.PHONY: clean
clean:
	rm -f *.o *.d

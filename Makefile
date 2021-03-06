PROJDIRS = .

CC = gcc
DEPFLAGS = -MM
CFLAGS = -c -g -std=c99 -O0
LDFLAGS = -g
INCLUDES := -I .
DEFINES := -D MACOS_CLASSIC
CFILES := $(shell find $(PROJDIRS) -type f -name "*.c")
OBJS := $(CFILES:.c=.o)

.PHONY: all link compile clean clean_intermediate

#link
all: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o shrinkwrap

-include $(OBJS:.o=.d)

#compile
%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) $*.c -o $*.o 
	$(CC) $(DEPFLAGS) $(DEFINES) $(INCLUDES) $*.c > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

clean: clean_intermediate
	rm -f shrinkwrap 

clean_intermediate:
	find . -type f -name '*.o' -exec rm {} +
	find . -type f -name '*.d' -exec rm {} +

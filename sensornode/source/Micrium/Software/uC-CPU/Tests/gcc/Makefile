

SOURCES_C=\
../../cpu_core.c \
../../Posix/GNU/cpu_c.c

FLAGS_C=\
 -O0 \
-g3 \
-Wall \
-c \
-fmessage-length=0

INCLUDE_C=\
-I.. \
-I../.. \
-I../../Posix/GNU \
-I../../../uC-LIB \
-I../../../uC-LIB/Cfg/Template



VPATH=\
../.. \
../../Posix/GNU/

LFLAGS=\


OBJECTS_C=$(notdir $(SOURCES_C:.c=.o)) 

OBJECTS= $(OBJECTS_C)


EXECUTABLE=hello

all: $(EXECUTABLE)
	
	
$(EXECUTABLE): $(OBJECTS)
	$(LD) $(OBJECTS)

$(OBJECTS_C): %.o: %.c
	$(CC) $(INCLUDE_C) $(FLAGS_C) -o $(notdir $@) $<

$(OBJECTS_S): %.o: %.S
	$(CC) $(INCLUDE_S) $(FLAGS_S) -o $(notdir $@) $<

.PHONY: clean


clean:
	@rm -f *.o
	@rm -f *.out
	@rm -f a.out

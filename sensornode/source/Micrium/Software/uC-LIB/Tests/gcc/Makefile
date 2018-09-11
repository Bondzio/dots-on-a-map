LD=echo
CFLAGS=\
-I.. \
-I../.. \
-I../../../uC-CPU \
-I../../../uC-CPU/Posix/GNU \
-c -g3 -Wall -Werror
#-O0 -emit-llvm -g3 -Wall -Wno-ignored-attributes -Werror -fsyntax-only


LDFLAGS=
LDPOSTFLAGS=

VPATH=\
../..

SOURCES=\
lib_ascii.c \
lib_math.c \
lib_mem.c \
lib_str.c


OBJECTS=$(notdir $(SOURCES:.c=.o))
EXECUTABLE=hello

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(LD) $(LDFLAGS) $(notdir $(OBJECTS)) $(LDPOSTFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $(notdir $@)

clean:
	rm $(OBJECTS)

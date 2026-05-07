CC := gcc
INSTALL := install
RM := rm
SRCDIR := src
CFLAGS := -O2 -Wall -I/usr/include/freetype2 -Iinclude
#CFLAGS := -O0 -g3 -ggdb -Wall -I/usr/include/freetype2 -Iinclude
LDFLAGS := -lvterm -lfreetype -lfontconfig -lSDL2 -lpthread
TARGET := bm_term
SOURCES := $(wildcard $(SRCDIR)/*.c)
OBJECTS := $(SOURCES:.c=.o)
LIBS := 

.PHONY: clean all install uninstall depend

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	$(RM) -f $(TARGET)
	$(RM) -f $(OBJECTS)

all: clean $(TARGET)

install: $(TARGET)
	mkdir -p /usr/local/libexec
	$(INSTALL) -m 755 $(TARGET) /usr/local/libexec
	$(INSTALL) -Tm 755 $(TARGET).sh /usr/local/bin/$(TARGET)

uninstall:
	$(RM) -f /usr/local/libexec/$(TARGET)
	$(RM) -f /usr/local/bin/$(TARGET)

depend:
	makedepend -Y -- $(CFLAGS) -- $(SOURCES)

# DO NOT DELETE

src/bmt_main.o: include/bmt/main.h
src/main.o: include/bmt/main.h

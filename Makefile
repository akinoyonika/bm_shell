CC := gcc
INSTALL := install
RM := rm
SRCDIR := src
CFLAGS := -O2 -Wall -I/usr/include/freetype2 -Iinclude -Itest/bm_term/include
#CFLAGS := -O0 -g3 -ggdb -Wall -I/usr/include/freetype2 -Iinclude -Itest/bm_term/include
LDFLAGS := -lvterm -lfreetype -lfontconfig
TARGET := bm_shell
SOURCES := $(wildcard $(SRCDIR)/*.c)
OBJECTS := $(SOURCES:.c=.o)
LIBS := 
TEST_SRCDIR := test
TEST_LDFLAGS := -lvterm -lfreetype -lSDL2 -lfontconfig -lpthread
TEST_TARGET := bms_test
TEST_SOURCES := $(wildcard $(TEST_SRCDIR)/*.c) $(wildcard $(SRCDIR)/bms_*.c) $(wildcard $(TEST_SRCDIR)/bm_term/src/bmt_*.c)
TEST_OBJECTS := $(TEST_SOURCES:.c=.o)

.PHONY: clean all test install uninstall depend target_test

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	$(RM) -f $(TARGET)
	$(RM) -f $(OBJECTS)
	$(RM) -f $(TEST_TARGET)
	$(RM) -f $(TEST_OBJECTS)

$(TEST_TARGET): $(TEST_OBJECTS)
	$(CC) $(TEST_OBJECTS) $(TEST_LDFLAGS) -o $@

test: $(TARGET) $(TEST_TARGET)
	./$(TEST_TARGET)

target_test: $(TARGET)
	bash -c 'TS=`stty -g`; stty -icanon -echo -isig; ./$(TARGET); stty $$TS'

all: clean $(TARGET)

install: $(TARGET)
	mkdir -p /usr/local/libexec
	$(INSTALL) -m 755 $(TARGET) /usr/local/libexec
	$(INSTALL) -Tm 755 $(TARGET).sh /usr/local/bin/$(TARGET)

uninstall:
	$(RM) -f /usr/local/libexec/$(TARGET)
	$(RM) -f /usr/local/bin/$(TARGET)

depend:
	makedepend -Y -- $(CFLAGS) -- $(SOURCES) -- $(wildcard $(TEST_SRCDIR)/*.c)

# DO NOT DELETE

src/bms_disp.o: include/bms/fff.h /usr/include/freetype2/ft2build.h
src/bms_disp.o: /usr/include/freetype2/freetype/config/ftheader.h
src/bms_disp.o: /usr/include/freetype2/freetype/freetype.h
src/bms_disp.o: /usr/include/freetype2/freetype/config/ftconfig.h
src/bms_disp.o: /usr/include/freetype2/freetype/config/ftoption.h
src/bms_disp.o: /usr/include/freetype2/freetype/config/ftstdlib.h
src/bms_disp.o: /usr/include/freetype2/freetype/config/integer-types.h
src/bms_disp.o: /usr/include/freetype2/freetype/config/public-macros.h
src/bms_disp.o: /usr/include/freetype2/freetype/config/mac-support.h
src/bms_disp.o: /usr/include/freetype2/freetype/fttypes.h
src/bms_disp.o: /usr/include/freetype2/freetype/config/ftconfig.h
src/bms_disp.o: /usr/include/freetype2/freetype/ftsystem.h
src/bms_disp.o: /usr/include/freetype2/freetype/ftimage.h
src/bms_disp.o: /usr/include/freetype2/freetype/fterrors.h
src/bms_disp.o: /usr/include/freetype2/freetype/ftmoderr.h
src/bms_disp.o: /usr/include/freetype2/freetype/fterrdef.h include/bms/font.h
src/bms_disp.o: include/bms/disp_xterm.h include/bms/disp.h
src/bms_disp_xterm.o: include/bms/disp_xterm.h
src/bms_fff.o: include/bms/fff.h
src/bms_font.o: /usr/include/freetype2/ft2build.h
src/bms_font.o: /usr/include/freetype2/freetype/config/ftheader.h
src/bms_font.o: /usr/include/freetype2/freetype/freetype.h
src/bms_font.o: /usr/include/freetype2/freetype/config/ftconfig.h
src/bms_font.o: /usr/include/freetype2/freetype/fttypes.h
src/bms_font.o: /usr/include/freetype2/freetype/ftsystem.h
src/bms_font.o: /usr/include/freetype2/freetype/ftimage.h
src/bms_font.o: /usr/include/freetype2/freetype/fterrors.h
src/bms_font.o: /usr/include/freetype2/freetype/ftmoderr.h
src/bms_font.o: /usr/include/freetype2/freetype/fterrdef.h include/bms/font.h
src/bms_main.o: /usr/include/freetype2/ft2build.h
src/bms_main.o: /usr/include/freetype2/freetype/config/ftheader.h
src/bms_main.o: /usr/include/freetype2/freetype/freetype.h
src/bms_main.o: /usr/include/freetype2/freetype/config/ftconfig.h
src/bms_main.o: /usr/include/freetype2/freetype/fttypes.h
src/bms_main.o: /usr/include/freetype2/freetype/ftsystem.h
src/bms_main.o: /usr/include/freetype2/freetype/ftimage.h
src/bms_main.o: /usr/include/freetype2/freetype/fterrors.h
src/bms_main.o: /usr/include/freetype2/freetype/ftmoderr.h
src/bms_main.o: /usr/include/freetype2/freetype/fterrdef.h include/bms/font.h
src/bms_main.o: include/bms/term.h include/bms/disp.h
src/bms_term.o: include/bms/term.h
src/main.o: include/bms/main.h
test/bms_disp_test.o: include/bms/disp.h test/bm_term/include/bmt/main.h
test/bms_fff_test.o: include/bms/fff.h
test/bms_font_test.o: /usr/include/freetype2/ft2build.h
test/bms_font_test.o: /usr/include/freetype2/freetype/config/ftheader.h
test/bms_font_test.o: /usr/include/freetype2/freetype/freetype.h
test/bms_font_test.o: /usr/include/freetype2/freetype/config/ftconfig.h
test/bms_font_test.o: /usr/include/freetype2/freetype/fttypes.h
test/bms_font_test.o: /usr/include/freetype2/freetype/ftsystem.h
test/bms_font_test.o: /usr/include/freetype2/freetype/ftimage.h
test/bms_font_test.o: /usr/include/freetype2/freetype/fterrors.h
test/bms_font_test.o: /usr/include/freetype2/freetype/ftmoderr.h
test/bms_font_test.o: /usr/include/freetype2/freetype/fterrdef.h
test/bms_font_test.o: include/bms/font.h
test/bms_main_test.o: include/bms/main.h test/bm_term/include/bmt/main.h

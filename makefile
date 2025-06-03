include config.mk

SRC = ./main.c
OBJ = ./main.o
OUT = ./minesweeper
MANPAGE = ./minesweeper.6

.PHONY: all clean install uninstall

all: $(OUT)

$(OUT): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CCFLAGS) -DVERSION='$(VERSION)' -c -o $@ $^

clean:
	rm -f $(OUT) $(OBJ)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(OUT) $(DESTDIR)$(PREFIX)/bin/$(OUT)
	chmod 775 $(DESTDIR)$(PREFIX)/bin/$(OUT)
	mkdir -p ${DESTDIR}${MANFIX}/man6
	sed "s/VERSION/${VERSION}/g" < $(MANPAGE) > ${DESTDIR}${MANFIX}/man6/$(MANPAGE)
	chmod 644 ${DESTDIR}${MANFIX}/man6/$(MANPAGE)

uninstall:
	rm -f	$(DESTDIR)$(PREFIX)/bin/$(OUT)\
		$(DESTDIR)$(MANFIX)/man6/$(MANPAGE)


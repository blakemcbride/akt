
LDFLAGS += -lutil

UPDATED_C := $(shell mktemp)
GENOCTBL := $(shell mktemp)
OCTBL := $(shell mktemp)

akt: akt.c
	chmod u+x $(GENOCTBL)
	$(CC) -DGENOCTBL $< -o $(GENOCTBL)
	$(GENOCTBL) > $(OCTBL)
	sed -e '/  \/\* !GEN! \*\//d;/OCTBL-BEGIN/r $(OCTBL)' $< > $(UPDATED_C)
	$(CC) -xc $(UPDATED_C) $(CFLAGS) $(LDFLAGS) -o $@
	mv $(UPDATED_C) $<
	rm $(GENOCTBL) $(OCTBL)

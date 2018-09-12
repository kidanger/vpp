CFLAGS ?= -march=native -O3 -Wall -Wextra
CFLAGS += -I.
LDLIBS = -lm

OBJ = vpp.o
BIN = example readvid writevid vp

BIN := $(addprefix bin/,$(BIN))

default: $(BIN)

bin/%: src/%.o $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

bin/readvid: src/readvid.o $(OBJ) src/iio.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)  -ltiff -ljpeg -lpng -lm
bin/writevid: src/writevid.o $(OBJ) src/iio.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)  -ltiff -ljpeg -lpng -lm
bin/vp: src/vp.o $(OBJ) src/iio.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)  -ltiff -ljpeg -lpng -lm


clean: ; @$(RM) $(BIN_ALL) bin/im src/*.o src/ftr/*.o src/misc/*.o
.PRECIOUS: %.o

DIRS = src
.deps.mk: ; for i in $(DIRS);do cc -I. -MM $$i/*.c|sed "\:^[^ ]:s:^:$$i/:g";done>$@
-include .deps.mk


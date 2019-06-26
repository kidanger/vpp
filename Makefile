CFLAGS ?= -march=native -O3 -Wall -Wextra
CFLAGS += -I.
LDLIBS = -lm

ifeq (,$(shell $(CC) $(CFLAGS) -dM -E -< /dev/null | grep __STDC_VERSION_))
CFLAGS := $(CFLAGS) -std=gnu99
endif

BIN = example readvid writevid vp vexec vlambda

BIN := $(addprefix bin/,$(BIN))

default: $(BIN)

bin/%: src/%.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

bin/readvid: src/readvid.o src/iio.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)  -ltiff -ljpeg -lpng -lm
bin/writevid: src/writevid.o src/iio.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)  -ltiff -ljpeg -lpng -lm
bin/vexec: src/vexec.o src/iio.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)  -ltiff -ljpeg -lpng -lm
bin/vlambda: src/vlambda.o src/iio.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)  -ltiff -ljpeg -lpng -lm


clean: ; @$(RM) ${BIN} src/*.o
.PRECIOUS: %.o

DIRS = src
.deps.mk: ; for i in $(DIRS);do cc -I. -MM $$i/*.c|sed "\:^[^ ]:s:^:$$i/:g";done>$@
-include .deps.mk


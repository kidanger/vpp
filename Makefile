
all: readvid writevid example

readvid: readvid.o vpp.o iio.o
	${CC} ${LDFLAGS} $^ -o $@ ${LDLIBS} -ltiff -ljpeg -lpng -lm

writevid: writevid.o vpp.o iio.o
	${CC} ${LDFLAGS} $^ -o $@ ${LDLIBS} -ltiff -ljpeg -lpng -lm

example: example.o vpp.o


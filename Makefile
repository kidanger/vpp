
LDFLAGS=-ltiff -ljpeg -lpng -lm

all: readvid writevid example

readvid: readvid.o vpp.o iio.o
writevid: writevid.o vpp.o iio.o

example: example.o vpp.o iio.o


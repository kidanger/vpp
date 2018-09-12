vpp
===

vpp is a video processing pipeline library.

```sh
$ make
$ ./readvid 'yourinput/*.png' - | ./example - - | ./writevid - output/%03d.tif
```

See `example.c` for an example of a recursive average using vpp.

A pipeline is defined as a sequence of programs communicating using vpp's C functions through unix pipes (or files).
A given program can have many inputs and outputs, allowing for complex pipelines.

To load and save a video, readvid and writevid are provided (but can be replaced with your own). They are using [iio](https://github.com/mnhrdt/iio) to load/save sequence of images.


Release notes
-------------

* v2.0.0 - 2018/09/12
	* removes the notion of video length (breaks API and file format compatibility with v1)

* v1.0.0 - 2018/09/11
	* first working version


TODO list
---------

For readvid and writevid:

* support multipage tiff
* support video format

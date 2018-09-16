vpp
===

vpp is a video processing pipeline library.

```sh
$ make
$ ./bin/readvid 'yourinput/*.png' - | ./bin/example - - | ./bin/writevid - output/%03d.tif
```

See `src/example.c` for an example of a recursive average using vpp.

A pipeline is defined as a sequence of programs communicating using vpp's C functions through unix pipes (or files).
A given program can have many inputs and outputs, allowing for complex pipelines.

To load and save a video, readvid and writevid are provided (but can be replaced with your own). They are using [iio](https://github.com/mnhrdt/iio) to load/save sequence of images.




Release notes
-------------

* v2.3.0 - 2018/09/16
	* add function 'vpp_init_inputs' to initialize multiple inputs at once, required for complex pipelines.
	* add two vp operators: dup and buf

* v2.2.0 - 2018/09/15
	* add 'vlambda' (from the code of [plambda](https://github.com/mnhrdt/imscript))
	* move 'vp exec' to 'vexec'
	* fix a bug where the libc kept the header in buffer
	
* v2.1.0 - 2018/09/13
	* add a 'vp' binary that contains many operators:
		* basic blocks: take, repeat, first, last, skip, concat, timeinterval, average, count, max, min, sum
		* more advanced: map, reduce, scan, framereduce, exec

* v2.0.0 - 2018/09/12
	* removes the notion of video length (breaks API and file format compatibility with v1)

* v1.0.0 - 2018/09/11
	* first working version


TODO list
---------

* documentation for the basic building blocks in `src/vp.c`

For readvid and writevid:

* support multipage tiff
* support video format

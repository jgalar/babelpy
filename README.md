BabelPy
=======

Proof of concept of a Python-based CTF event filtering system. This is primarily an experiment in
calling Python code from C, but provides a powerful filtering mechanism over Babeltrace.

### Dependency
* [Python 3.0+](http://www.python.org)
* [SWIG 2.0+](http://www.swig.org)
* [Babeltrace 1.2+](http://www.efficios.com/babeltrace)

### Usage
````
$ ./babelpy /path/to/ctf/trace
````

You may customize your filter by editing filter.py and tweak the output formatting in formatter.py.

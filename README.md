babelpy
=======

Proof of concept of a Python-based CTF event filtering system. This is primarily an experiment in
calling Python code from C but provides an interesting filtering mechanism over Babeltrace.

### Dependency
Requires [Python 3.x](http://www.python.org), SWIG 2.x+[http://www.swig.org] and Babeltrace[http://www.efficios.com/babeltrace].

### Usage
````
$ ./filtertest /path/to/ctf/trace
````

You may customize your filter by editing filter.py and tweak the output formatting in formatter.py.

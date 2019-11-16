PYTHON OPCODES
===

This is the porting to Python3.x of the Python opcodes of Csound which ran under Python 2.7 until know.
The backward compatibility is preserved for the Csound code of the orchestra, however it is not maintained
for the Python code that would be embedded in a Csound orchestra or in Csound instruments, due to the lack
of backward compatibility between Python 3.x and Python 2.7.

Python 2.7 will not be maintained past 2020. The
[official porting guide](https://docs.python.org/3/howto/pyporting.html) has advice for
running Python 2 code in Python 3.


Build Instructions
---

The build requires Csound to be installed, as well as CMake, and Python 3.x. With this
in place, you can do (from the py dir):

```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

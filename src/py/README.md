Python opcodes
===

This is the porting to Python 3.x of the Python opcodes of Csound which ran under Python 2.7 until know.
The backward compatibility is preserved for the Csound code of the orchestra, however it is not maintained
for the Python code that would be embedded in a Csound orchestra or in Csound instruments, due to the lack
of backward compatibility between Python 3.x and Python 2.7.

Python 2.7 will not be maintained past 2020. The
[official porting guide](https://docs.python.org/3/howto/pyporting.html) has advice for
running Python 2 code in Python 3.

To build these opcodes, you will need to install Python from
[www.python.org](www.python.org), or the devel package (on linux).

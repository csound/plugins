PYTHON OPCODES FOR CSOUND
===
By Maurizio Umberto Puxeddu.<br>
Modified by Michael Gogins for Csound 5, 2004 July 24.<br>
Modified by François Pinot for using with Python 3.x, 2019 November.<br>

BEFORE COMPILING
---
To generate the source code for the Python opcodes just run the **pycall-gen.py**
and **pyx-gen.py** scripts in this directory.

This way of doing had been broken by adding different modifications directly to
the **pycall.auto.c** and **pyx.auto.c** files. These modifications have been integrated
back into the **pycall-gen.py** and **pyx-gen.py** scripts.
So if one has to modifiy the opcodes, one should do this in the two scripts
**pycall-gen.py** and **pyx-gen.py**, and then regenerate the
opcodes files by running these scripts.

NOTES
---
These opcodes require the Python virtual machine to be initialized, which 
can be done by putting the `pyinit` opcode in the orchestra header.

See the [Python opcodes pages](https://csound.com/docs/manual/py.html) of the Csound manual,
and examples in the **examples** directory.

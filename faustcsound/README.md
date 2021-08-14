Faust opcodes
=======

To build the faust opcodes, you will need the Faust library, which can downloaded
from [https://github.com/grame-cncm/faust/releases] (https://github.com/grame-cncm/faust/releases)
or built from sources (you will also need libcrypto and LLVM
installed).

If using the pre-built binaries the following CMake variables are
important:

`FAUST_INCLUDE_DIR_HINT`

and

`FAUST_LIB_DIR_HINT`

They should point to the include and lib directories inside the faust
binary package. For example, if the package (for example,
Faust 2.33.1) is placed at the top-level user directory use the CMake command

```
cmake .. -DFAUST_INCLUDE_DIR_HINT=$HOME/faust-2.33.1/include
-DFAUST_LIB_DIR_HINT=$HOME/faust-2.33.1/lib
```

to correctly find the library and headers. The library will be built
and linked to the installed faust library.


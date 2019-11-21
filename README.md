CSOUND PLUGINS
===

This repository will contain the code for Csound plugins which were in the main repository, and the code for new plugins as well.
This is actually very experimental and it should become effective with Csound7.

Build Instructions for Linux
---

The build requires Csound to be installed, as well as CMake. With this
in place, you can do :

```
$ mkdir build
$ cd build
$ cmake ../
$ make
$ sudo make install
$ sudo ldconfig
```

By default, all the plugins are built. If one want to excludes a plugin from the build process, one can pass an option to the `cmake` command. For example, to exclude the chua plugin, the `cmake` command would be:
```
$ cmake -DBUILD_CHUA_OPCODES=OFF ../

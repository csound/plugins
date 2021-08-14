CSOUND PLUGINS
===

This repository contains the code for Csound plugins that require
external dependencies, as well as code for new third-party contributed
plugins. This is actually very experimental and it should become fully
effective in version 7 of Csound.

Currently, the plugins available in this tree are

- AbletonLive Link (requires Ableton link)
 link_create link_enable link_is_enabled link_tempo_set link_tempo_get link_beat_get link_metro link_beat_request link_beat_force

- chua (requires Eigen library, header-only)
 chuap

- Faust (requires libfaust)
 faustgen faustcompile faustaudio faustdsp faustplay faustctl

- image (requires libpng)
 imageload imagesave imagecreate imagesize imagegetpixel imagesetpixel imagefree

- py (requires a Python 3.x installation with dev libs)
 Many opcodes to call python code

- widgets (requires the FLTK LIB)
 FLTK-based widgets. These opcodes do not work correctly on MacOS due
 to incompatibilities with the operating system.

Build Instructions for Linux
---

The build requires Csound to be installed, as well as CMake. With this
in place, you can do :

```
$ git clone https://github.com/csound/plugins.git
$ cd plugins
$ mkdir build
$ cd build
$ cmake ../
$ make
```

By default, all the plugins are built. If one wants to exclude a plugin from the build process, one can pass an option to the cmake command. For example, to exclude the chua plugin, the `cmake` command would be:

```
$ cmake -DBUILD_CHUA_OPCODES=OFF ../
```

To install the opcodes you have built

```
$ make install
$ ldconfig
```

Depending on your permissions, you might need to prepend `sudo` to
these commands. After the first build the plugins can be updated with

```
$ git pull
$ make
$ make install
```

Build Instructions for MacOS
---

The build requires Csound to be installed either from the binary
releases, or from a user build, as well as CMake. With these
in place, you can do :

```
$ git clone https://github.com/csound/plugins.git
$ cd plugins
$ mkdir build
$ cd build
$ cmake ../
$ make
```

By default, all the plugins are built. If one wants to exclude a
plugin from the build process, one can pass an option to the cmake
command. For example, to exclude the chua plugin, the cmake command
would be:

```
$ cmake -DBUILD_CHUA_OPCODES=OFF ../
```

To install the plugins, you can do

```
$ make install
```

CMake will place the plugins in your installed framework. Depending on
permissions, you might need to use `sudo`

```
$ sudo make install
```

This is the case if you are using the released Csound binaries.
After the first build the plugins can be updated with 

```
$ git pull
$ make
$ make install
```

using `sudo` in the last step if needed.

CSOUND PLUGINS
===

This repository will contain the code for Csound plugins which were in the main repository, and the code for new plugins as well.
This is actually very experimental and it should become effective with Csound7.

The plugins available in this tree are

- AbeltonLive Link
 link_create link_enable link_is_enabled link_tempo_set link_tempo_get link_beat_get link_metro link_beat_request link_beat_force

- chua
 chuap

- Faust
 faustgen faustcompile faustaudio faustdsp faustplay faustctl

- image
 imageload imagesave imagecreate imagesize imagegetpixel imagesetpixel imagefree

- py
 Many opcodes to call python code

- widgets
 FLTK-based widgets

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
$ sudo make install
$ sudo ldconfig
```

By default, all the plugins are built. If one wants to exclude a plugin from the build process, one can pass an option to the cmake command. For example, to exclude the chua plugin, the `cmake` command would be:

```
$ cmake -DBUILD_CHUA_OPCODES=OFF ../
```

After the first build the plugins can be updated with

```
$ git pull
$ make
$ make install
```

Build Instructions for MacOS
---

The build requires Csound to be installed, as well as CMake. With these
in place, you can do :

```
$ git clone https://github.com/csound/plugins.git
$ cd plugins
$ mkdir build
$ cd build
$ cmake ../
$ make
$ sudo make install
```

By default, all the plugins are built. If one wants to exclude a plugin from the build process, one can pass an option to the cmake command. For example, to exclude the chua plugin, the cmake command would be:
```
$ cmake -DBUILD_CHUA_OPCODES=OFF ../
```

After the first build the plugins can be updated with

```
$ git pull
$ make
$ make install
```

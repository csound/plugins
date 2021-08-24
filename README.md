CSOUND PLUGINS
===

This repository contains the code for Csound plugins that require
external dependencies, as well as code for new third-party contributed
plugins. This is actually very experimental and it should become fully
effective in version 7 of Csound.

Currently, the plugins available in this tree are

- AbletonLive Link (requires Ableton link)  
 **link_create link_enable link_is_enabled link_tempo_set link_tempo_get link_beat_get link_metro link_beat_request link_beat_force**

- chua (requires Eigen library, header-only)  
 **chuap**

- Faust (requires libfaust)  
 **faustgen faustcompile faustaudio faustdsp faustplay faustctl**

- image (requires libpng)  
 **imageload imagesave imagecreate imagesize imagegetpixel imagesetpixel imagefree**

- py (requires a Python 3.x installation with dev libs)  
 **Many opcodes to call Python code**

- widgets (requires the FLTK LIB)  
**FLTK-based widgets.**  
NB: These opcodes do not work correctly on MacOS due
to incompatibilities with the operating system.

- virtual keyboard (requires the FLTK LIB)  
**Virtual MIDI keyboard midi backend**

- STK opcodes (requires STK library)  
**Physical model opcodes using the STK library**


Install location
--------------
The CMake scripts in this repository use the default CS_USER_PLUGIN
location as defined in the Csound build. These are:

- LINUX: `$HOME/.local/lib/csound/${APIVERSION}/plugins64`(doubles)  
          or `$HOME/.local/lib/csound/${APIVERSION}/plugins` (floats)  
- MACOS: `$HOME/Library/csound/${APIVERSION}/plugins64` (doubles)  
         or `$HOME/Library/csound/${APIVERSION}/plugins` (floats)  
- Windows:  `%LOCALAPPDATA%\csound\${APIVERSION}\plugins64`(doubles)  
        or `%LOCALAPPDATA%\csound\${APIVERSION}\plugins` (floats)


Build Instructions for Linux and MacOS
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

By default, all the plugins are built. If one wants to exclude a
plugin from the build process, one can pass an option to the cmake command.
For example, to exclude the chua plugin, the `cmake` command would be:

```
$ cmake -DBUILD_CHUA_OPCODES=OFF ../
```

For the FLTK dependent plugins, the configuration variable used is
`USE_FLTK`.

To install the opcodes you have built

```
$ make install
```

Depending on your permissions, you might need to prepend `sudo` to
these commands. After the first build the plugins can be updated with

```
$ git pull
$ make
$ make install
```

using `sudo` in the last step if raised permissions are needed.

Csound Location
------------
CMake will normally find the installed Csound headers (and library)
automatically. However, if your Csound headers and library are not
placed in the usual locations, you can use the following CMake option variables
to tell CMake where they are:

```
CSOUND_INCLUDE_DIR_HINT
```
and

```
CSOUND_LIBRARY_DIR_HINT
```

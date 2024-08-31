Plugins for Csound 6.x
=======

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
location on MacOS and Windows as defined in the Csound build, or a
library instalation directory (customisable) on LINUX. These are:

- LINUX: depends on both `CMAKE_INSTALL_PREFIX` and `USE_LIB64`. It is then installed in:
   * if `USE_LIB64=1` then to
       * for doubles: `${CSOUND_INSTALL_PREFIX}/lib64/csound/plugins64-${APIVERSION}`
       * for floats: `${CSOUND_INSTALL_PREFIX}/lib64/csound/plugins-${APIVERSION}` (floats)`
   * if `USE_LIB64=0` then to
       * for doubles: `${CSOUND_INSTALL_PREFIX}/lib/csound/plugins64-${APIVERSION}`
       * for floats: `${CSOUND_INSTALL_PREFIX}/lib/csound/plugins-${APIVERSION}`

- MACOS: 
    * For doubles: `$HOME/Library/csound/${APIVERSION}/plugins64` 
    * For floats: `$HOME/Library/csound/${APIVERSION}/plugins`
- Windows: 
    * For doubles: `%LOCALAPPDATA%\csound\${APIVERSION}\plugins64`
    * For floats: `%LOCALAPPDATA%\csound\${APIVERSION}\plugins`


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

using `sudo` in the last step if raised permissions are needed. On
Linux, the installation location can be set with the relevant CMake
variables as indicated above.

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

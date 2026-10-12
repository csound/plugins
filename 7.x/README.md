Plugins for Csound 7.x
=======

Currently, the plugins available in this tree are

- chua (requires the Eigen library, header-only)  
**chuap**

- fluidOpcodes (requires fluidsynth)  
**fluidEngine fluidLoad fluidNote fluidCCi fluidCCk fluidControl fluidProgramSelect fluidAllOut fluidOut fluidInfo fluidSetInterpMethod**

- image (requires libpng)  
**imageload imagesave imagecreate imagesize imagegetpixel imagesetpixel imagefree**

- hdf5 (requires the HDF5 library)  
**hdf5read hdf5write**

- linear_algebra (requires the gmm++ library, header-only; vendored under this directory)  
**A large family of `la_*` opcodes, with i-rate and k-rate variants, for vector, matrix and tensor operations**

- py (requires a Python 3.x installation with dev libs)  
**Many opcodes to call Python code**

- websockets (requires libwebsockets)  
**websocket**

- wiimote (requires the wiiuse library; CMake-only, see note below)  
**wiiconnect wiidata wiisend wiirange**


Fluid settings opcodes
---

Besides the engine/SoundFont/MIDI opcodes listed above, `fluidOpcodes`
provides four opcodes to read and write the settings of an engine created by
`fluidEngine` (resolved through `fluid_synth_get_settings`). In each case the
first argument is the engine id and the second is the setting name:

| Opcode | Outputs | Inputs | Description |
| --- | --- | --- | --- |
| `fluidSettingNum` | – | `iSk` | set an int/num/bool setting |
| `fluidSettingString` | – | `iSS` | set a string setting |
| `fluidSettingGetNum` | `k` | `iS` | read a numeric setting |
| `fluidSettingGetString` | `S` | `iS` | read a string setting |

The setters fail at init if the setting name is unknown or of the wrong type;
the getters report a message and return 0 (or an empty string) instead.


Wiimote opcodes
---

The `wiimote` opcodes (`wiiconnect`, `wiidata`, `wiisend`, `wiirange`) are a
CMake-only port of the Csound 6.x plugin. They are **not** built by the CI and
are **not** included in the release packages, because the required `wiiuse`
library is not available as a standard distribution package: it is not in the
Ubuntu repositories, and the vcpkg `wiiuse` port ships only pkg-config
metadata, so it cannot be wired into the automated builds.

When `wiiuse` (headers and library) is found by CMake, the plugin is built and
installed like any other; otherwise `BUILD_WIIMOTE_OPCODES` is silently
disabled. To build it locally, install `wiiuse` and configure with
`-DBUILD_WIIMOTE_OPCODES=ON`.


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
The options are `BUILD_CHUA_OPCODES`, `BUILD_FLUID_OPCODES`,
`BUILD_IMAGE_OPCODES`, `BUILD_HDF5_OPCODES`, `BUILD_LINEAR_ALGEBRA_OPCODES`,
`BUILD_WIIMOTE_OPCODES`, `BUILD_WEBSOCKET_OPCODE` and
`BUILD_PYTHON_OPCODES`. For example, to
exclude the chua plugin, the `cmake` command would be:

```
$ cmake -DBUILD_CHUA_OPCODES=OFF ../
```


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

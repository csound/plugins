CSOUND PLUGINS
===

This repository contains the code for Csound 7 plugins that require
external dependencies, as well as code for new third-party contributed
plugins. Native plugins build with CMake; the WASM package `@csound/plugins`
uses Nix and CMake and is documented in [`wasm/README.md`](wasm/README.md).

Csound 6 is end-of-life. Its plugin tree is archived on the
[`csound6` branch](https://github.com/csound/plugins/tree/csound6).

Currently, the plugins available in this tree are

- chua (requires the Eigen library, header-only)  
**chuap**

- fluidOpcodes (requires fluidsynth)  
**fluidEngine fluidLoad fluidNote fluidCCi fluidCCk fluidControl fluidProgramSelect fluidAllOut fluidOut fluidInfo fluidSetInterpMethod**

- dssi4cs (hosts native DSSI/LADSPA plugins; Linux only, requires the ALSA headers)  
**dssiinit dssiactivate dssiaudio dssisynth dssictls dssiget dssinote dssievent dssinrpn dssiprogram dssiconfigure dssiprograminfo dssiinfo dssilist**

- faust (requires the Faust compiler library and headers, `libfaust`)  
**faustgen faustcompile faustaudio faustdsp faustplay faustctl**

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

- p5glove (requires the p5glove driver; CMake-only, see note below)  
**p5gconnect p5gdata**

- CUDA (requires the CUDA toolkit; Linux only, disabled by default, see note below)  
**cudasynth cudanal cudasliding cudaconv cudapconv**

- OpenCL (requires the OpenCL headers/loader; see note below)  
**cladsyn**

- STK (port of the Synthesis ToolKit instruments; STK 4.6.2 is vendored under this directory, see note below)  
**STKBandedWG STKBeeThree STKBlowBotl STKBlowHole STKBowed STKBrass STKClarinet STKDrummer STKFlute STKFMVoices STKHevyMetl STKMandolin STKModalBar STKMoog STKPercFlut STKPlucked STKResonate STKRhodey STKSaxofony STKShakers STKSimple STKSitar STKStifKarp STKTubeBell STKVoicForm STKWhistle STKWurley**  


WebAssembly (WASM) plugins
---

Besides the native plugin libraries described above, this tree can also be
built for the browser: Csound 7 runs under WebAssembly (WASI) through the
`@csound/browser` runtime, and a subset of these plugins is available as WASM
side modules. The WASM build is driven by the Nix expression in
[`wasm/default.nix`](wasm/default.nix) and published as the `@csound/plugins`
npm package; see [`wasm/README.md`](wasm/README.md) for the build and test
instructions.

Only four plugins are built for WASM:

- chua (`chuap`)
- image
- linear_algebra
- STK (`stkops`)

The remaining plugins are **disabled** in the WASM build, either because their
dependencies do not exist in the WASI/static-PIC toolchain or because they need
platform facilities the browser does not provide: fluidOpcodes (FluidSynth),
hdf5, faust (libfaust), websockets, py (Python), wiimote, p5glove, CUDA and
OpenCL.

The `@csound/plugins` package exposes one asset URL per WASM plugin:

| Export | Plugin | File |
| --- | --- | --- |
| `plugins.chua` | chua | `chua.wasm` |
| `plugins.image` | image | `image.wasm` |
| `plugins.linearAlgebra` | linear_algebra | `linear_algebra.wasm` |
| `plugins.stk` | STK | `stkops.wasm` |

The URLs are passed to the runtime's `withPlugins` option, e.g.:

```js
import { Csound } from "@csound/browser";
import { plugins } from "@csound/plugins";

const csound = await Csound({ withPlugins: [plugins.chua, plugins.image] });
```

Unlike the native build, the WASM STK plugin embeds its raw-wave samples in
`stkops.wasm` (`STK_EMBED_RAWWAVES` defaults to ON for WASM): at initialization
it writes them to Csound's virtual filesystem and sets the wave path, so no
separate download or `RAWWAVE_PATH` setup is needed. An explicit `RAWWAVE_PATH`
still overrides the embedded samples.


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


P5Glove opcodes
---

The `p5glove` opcodes (`p5gconnect`, `p5gdata`) are a CMake-only port of the
Csound 6.x plugin. They are **not** built by the CI and are **not** included in
the release packages, because the required P5Glove driver is not widely
packaged: there is no vcpkg port for `libp5glove`, and its Linux backend needs
the legacy libusb-0.1 API (not present in vcpkg).

When an external `libp5glove` (headers and library) is found by CMake, the
plugin is built and installed like any other; otherwise
`BUILD_P5GLOVE_OPCODES` is silently disabled. On macOS the driver can
additionally be built from the sources bundled under `src/p5glove/libp5glove`
by configuring with `-DP5GLOVE_USE_BUNDLED=ON` (in addition to
`-DBUILD_P5GLOVE_OPCODES=ON`).


CUDA opcodes
---

The `CUDA` opcodes (`cudasynth`, `cudanal`, `cudasliding`, `cudaconv`,
`cudapconv`) are a port of the Csound 6.x plugin to the Csound 7 plugin API.
They run their DSP kernels on an NVIDIA GPU; `cudanal` and `cudapconv` also
rely on cuFFT.

They are **disabled by default** and only built when the CUDA toolkit is
found: configure with `-DBUILD_CUDA_OPCODES=ON`. CUDA is **not available on
macOS** (NVIDIA dropped macOS support after 10.13), so on macOS the option is
ignored. The Linux CI builds them compile-only (the hosted runners have no
GPU) and they are included in the Linux release package. Windows is not built
or tested, so the CUDA opcodes are currently Linux-only.

Building them requires CMake 3.18 or later; with an older CMake the option
is ignored. The compute capabilities default to `75 80 86 89`; override with
`-DCMAKE_CUDA_ARCHITECTURES="..."` (for example `native` for the local GPU).


OpenCL opcodes
---

The `OpenCL` opcode (`cladsyn`) is a port of the Csound 6.x plugin to the
Csound 7 plugin API. It runs its DSP kernels on any available OpenCL device
(CPU or GPU) through the ICD loader.

It is built by default when OpenCL is available: the system framework is used
on macOS, `ocl-icd-opencl-dev` on Linux and the vcpkg `opencl` port on Windows.
Configure with `-DBUILD_OPENCL_OPCODES=OFF` to exclude it. The CI builds it
compile-only (the hosted runners have no OpenCL device) and it is included in
the Linux, macOS and Windows release packages.


STK opcodes
---

The `STK` opcodes are a port of the Csound 6.x plugin to the Csound 7 plugin
API. They expose the instruments of Perry Cook and Gary Scavone's Synthesis
ToolKit in C++ (STK), each named `STK` followed by the STK class, e.g.
`STKSitar`. All of them share the same signature:

```
aout STKName ifrequency igain {kcontroller0, kvalue0, ..., kcontrollerN, kvalueN}
```

The STK sources are **vendored under this directory** (`include/`, `src/` and
`rawwaves/`) and built from source, so no external STK installation is needed
and the result is independent of any system or package-manager STK. The
vendored copy carries a fix for the sample buildup reported in
[csound/plugins#4](https://github.com/csound/plugins/issues/4): the noise
modulation of the sitar delay line in `Sitar::setFrequency`, which could make
`STKSitar` build up clipped output over time, has been removed.

It is built by default; configure with `-DBUILD_STK_OPCODES=OFF` to exclude
it. The wavetables the instruments need are installed alongside the plugin
(under `stk/rawwaves`). At run time the `RAWWAVE_PATH` environment variable,
if set, overrides the built-in default location; otherwise the installed
`rawwaves` directory is used. The plugin is included in the Linux, macOS and
Windows release packages.


Faust opcodes
---

The `faust` opcodes embed the [Faust](https://faust.grame.fr/) compiler and
DSP runtime into Csound, so a Faust program supplied as a string is compiled
at init time and run as a Csound opcode. They are a port of the Csound 6.x
plugin to the Csound 7 plugin API:

```
ihandle[,asig1,...]  faustgen     Scode[,ain1,...]
ihandle              faustcompile Scode, Sargs[, iasync, istacksize[, Starget]]
ihandle[,asig1,...]  faustaudio   ifactory[,ain1,...]
ihandle              faustdsp     ifactory
asig1[,asig2,...]    faustplay    ihandle[,ain1,...]
                     faustctl     ihandle, Slabel, kval[, Slabel1, kval1, ...]
```

Where:

- `faustgen` compiles `Scode` and runs it in a single opcode, returning a
  handle and the audio outputs.
- `faustcompile` compiles `Scode` and returns `ihandle`, a handle to the
  compiled factory, to be passed to `faustaudio` or `faustdsp`.
- `faustaudio`/`faustdsp` instantiate the factory named by `ifactory`;
  `faustaudio` has audio inputs and outputs, `faustdsp` has none.
- `faustplay` runs the DSP instance named by `ihandle` (one returned by
  `faustgen`, `faustaudio` or `faustdsp`) with explicit inputs and outputs.
- `faustctl` sets the control `Slabel` of a DSP instance; both an i-rate
  (`iSi`) and a k-rate (`iSk`) form are provided, the latter updating the
  control every k-cycle.

`Scode` is the Faust program (programs that use the standard library need
`import("stdfaust.lib");`). `Sargs` holds any extra Faust compiler options;
`iasync` selects asynchronous compilation (on by default), `istacksize` is
accepted for backwards compatibility but ignored (the Csound 7 threading API
no longer sets the thread stack size) and `Starget` is an optional LLVM
target string (an empty string selects the native target).

The opcodes need the Faust compiler library (`libfaust`) and headers. The
build looks for them automatically; if Faust is in a non-standard location,
point CMake at it with `FAUST_INCLUDE_DIR_HINT`, `FAUST_LIB_DIR_HINT` and
`FAUST_STDLIB_DIR_HINT`. With `-DFAUST_STATIC=ON` the plugin is linked
against the self-contained `libfaustwithllvm` archive, which bundles LLVM so
the plugin has no runtime dependency on `libfaust` (at the cost of a much
larger plugin).

It is built by default; configure with `-DBUILD_FAUST_OPCODES=OFF` to
exclude it. The Faust standard library is installed next to the plugin (in a
`faust` subdirectory), so `import("stdfaust.lib")` works without a separate
Faust installation; set `FAUST_LIB_PATH` at run time to override it. The CI
downloads the official Faust release binaries, builds with `FAUST_STATIC=ON`
and includes the plugin (and the standard library) in the Linux, macOS and
Windows release packages.


DSSI and LADSPA opcodes
---

The `dssi4cs` plugin hosts native DSSI synths and LADSPA effects:

```
ih                    dssiinit      Sfilename, iindex [, iverbose]
                      dssilist
                      dssiinfo      ih
                      dssiactivate  ih, kactive
a1[,a2,...]           dssisynth     ih [, ain1, ...]
a1[,a2,...]           dssiaudio     ih [, ain1, ...]
                      dssinote      ktrigger, ih, knote, kvelocity, kduration [, ichannel]
                      dssievent     ktrigger, ih, knote, kvelocity
                      dssievent     ktrigger, ih, kstatus, kchannel, kdata1, kdata2 [, koffset]
                      dssinrpn      ktrigger, ih, kchannel, kparameter, kvalue [, koffset]
                      dssictls      ih, iport, kvalue, ktrigger
kvalue                dssiget       ih, iport
                      dssiprogram   ih, kbank, kprogram, ktrigger
Sname, ibank, iprogram dssiprograminfo ih, iindex
                      dssiconfigure ih, Skey, Svalue
```

The opcodes load a plugin library, instantiate a DSSI synth or LADSPA effect,
feed it Csound audio and MIDI events, and return its audio. Libraries are
looked up by path or through `DSSI_PATH` and `LADSPA_PATH`. See the
[module README](src/dssi4cs/README.md) for the full opcode reference.

Hosting native plugins is Linux-only: the opcodes load the shared objects with
`dlopen` and use the ALSA sequencer event types, so they need the ALSA headers
(`libasound2-dev` on Debian/Ubuntu) but not the ALSA library or a running
sequencer. The DSSI and LADSPA API headers are **vendored under this
directory** (in `src/dssi4cs/include`), so no system DSSI/LADSPA development
package is required.

It is built by default on Linux when the ALSA headers are found and silently
disabled elsewhere; configure with `-DBUILD_DSSI_OPCODES=OFF` to exclude it.
It is included in the Linux release package only.


Releasing
--------------

[`package.json`](package.json) holds the plugin version for both native and npm
packages. GitHub release tags and asset names use that version as written,
without a `v` prefix. The workflow's `CSOUND_VERSION` selects the Csound build
used to compile and test native plugins; it does not set the plugin version.

1. Set the next version with `npm version <version> --no-git-tag-version` and
   commit both `package.json` and `package-lock.json`. The workflow checks that
   their versions match. Use a stable version for a release from `main`, or a
   prerelease version for a release from `develop`.
2. Open a PR into the release branch (normally `develop` into `main`) and add
   the `release` label before merging it.
3. Merge the PR. Once all native build jobs pass, the push workflow publishes
   the release and tags that exact merge commit. A prerelease version creates
   a GitHub prerelease.

PR builds, manual runs, and pushes without a matching merged PR carrying the
`release` label only build and test. New pushes do not cancel release builds.
To retry a failed release, rerun its original workflow run. A tag that already
points to the same commit is safe to reuse; if it points elsewhere, choose a
new version. The workflow never moves an existing tag.

npm publication remains a separate manual step; see the
[npm release instructions](wasm/README.md#npm-releases).


Release binaries and macOS architecture
--------------

The CI builds each plugin on its default hosted runner and packages the
resulting plugin libraries into the GitHub release assets. These binaries are
single-architecture — they are **not** universal fat binaries.

On macOS the release plugins are built on the Apple Silicon (`arm64`)
`macos-latest` runner with no `CMAKE_OSX_ARCHITECTURES` override, and the
dependencies come from vcpkg using the `arm64-osx` triplet. The
`csound7-plugins-*-macos.zip` release asset therefore contains **arm64-only**
`.dylib` files: they load in the arm64 build of Csound on Apple Silicon, but
not in the x86_64 (Intel) build on Intel Macs. Users on Intel Macs should
build the plugins from source against x86_64 versions of their dependencies
(see the build instructions below).


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

On all platforms, the install directory can be overridden by setting the
`PLUGIN_INSTALL_DIR` CMake cache variable (e.g. `-DPLUGIN_INSTALL_DIR=/custom/path`).


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
The options are `BUILD_CHUA_OPCODES`, `BUILD_CUDA_OPCODES`,
`BUILD_DSSI_OPCODES`, `BUILD_OPENCL_OPCODES`, `BUILD_FAUST_OPCODES`,
`BUILD_FLUID_OPCODES`, `BUILD_IMAGE_OPCODES`, `BUILD_HDF5_OPCODES`,
`BUILD_LINEAR_ALGEBRA_OPCODES`, `BUILD_P5GLOVE_OPCODES`, `BUILD_STK_OPCODES`,
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

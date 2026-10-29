Faust opcodes
=======

These opcodes embed the [Faust](https://faust.grame.fr/) compiler and DSP
runtime into Csound:

* `faustgen` - compiles and runs a single Faust program
* `faustcompile` - compiles a Faust program, returning a handle
* `faustaudio` - creates a DSP instance from a compiled program
* `faustdsp` - creates a DSP instance (no audio outputs) from a compiled program
* `faustplay` - runs an existing DSP instance with explicit inputs/outputs
* `faustctl` - sets the value of a control of a DSP instance

Building
--------

To build the Faust opcodes you need the Faust library (headers and
`libfaust`), which can be downloaded as a binary package from
<https://github.com/grame-cncm/faust/releases> or built from source
(which also requires libcrypto and LLVM).

When using the pre-built binary packages the following CMake variables
are important:

* `FAUST_INCLUDE_DIR_HINT` - the `include` directory of the package
  (contains `faust/dsp/llvm-dsp.h`).
* `FAUST_LIB_DIR_HINT` - the `lib` directory of the package.
* `FAUST_STDLIB_DIR_HINT` - the directory containing `stdfaust.lib`
  (`share/faust` in the binary packages, `libraries` in the source tree).
* `FAUST_STATIC` - when `ON`, link against the self-contained
  `libfaustwithllvm` archive, which bundles LLVM so the plugin has no
  runtime dependency on `libfaust`. This produces a much larger plugin.

For example, if Faust 2.88.0 is placed in the top-level user directory:

```
cmake .. -DFAUST_INCLUDE_DIR_HINT=$HOME/faust-2.88.0/include \
         -DFAUST_LIB_DIR_HINT=$HOME/faust-2.88.0/lib \
         -DFAUST_STDLIB_DIR_HINT=$HOME/faust-2.88.0/share/faust \
         -DFAUST_STATIC=ON
```

Without `FAUST_STATIC` the plugin is linked to `libfaust` dynamically and
`libfaust` must be locatable at runtime (the build sets an rpath to the
library directory by default). With `FAUST_STATIC=ON` the LLVM/Faust
symbols are linked in and hidden, so only the opcode entry points are
exported.

The Faust standard library is installed next to the plugin (in a `faust`
subdirectory of the plugin directory) and the plugin looks for it there,
so `import("stdfaust.lib")` works without a separate Faust installation.
Set `FAUST_LIB_PATH` to override this, e.g.

```
export FAUST_LIB_PATH=$HOME/faust-2.88.0/share/faust
```

Faust programs that use the standard library (e.g. `os.osc`) need
`import("stdfaust.lib");`.

Notes on the Csound 7 port
--------------------------

* `MYFLT` was replaced by `cs_float` and `FAUSTFLOAT` is defined to
  `cs_float`, so the plugin always matches the Csound sample type.
* Opcode cleanup uses the `OENTRY` `deinit` field instead of the removed
  `RegisterDeinitCallback`; `faustcompile` still uses a reset callback so
  that compiled factories outlive the compiling instrument.
* `CreateThread2` was removed from the Csound API, so the `istacksize`
  argument of `faustcompile` is now accepted but ignored. `faustgen`
  compiles synchronously on the calling (init) thread, and asynchronous
  `faustcompile` uses the default thread stack size.

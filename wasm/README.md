# @csound/plugins

WASI Preview 2 opcode plugins for Csound 7.

| Entry point | Target | Samples | Assets |
| --- | --- | --- | --- |
| `@csound/plugins/wasm32` | wasm32-wasip2 | float | `chua.wasm`, etc. |
| `@csound/plugins/wasm64` | wasm64-wasip2 | double | `chua-wasm64.wasm`, etc. |

`@csound/plugins` selects wasm32 float, matching the default browser engine.
Each entry exports one `plugins` object. Both builds use the same plugin sources.

This package includes Chua (`chuap`), image, linear algebra, and STK. This build
disables FluidSynth, HDF5, Python, WebSockets, device, and GPU plugins.
Csound 6 is not supported.

## Runtime dependency

This change depends on [csound/csound#3556](https://github.com/csound/csound/pull/3556).
The npm versions in `package.json` predate that change. Until matching releases
exist, use its built source directory as described below. CI pins the upstream
commit and builds its engines and SDKs from source.

A plugin's pointer width, sample types, and memory mode must match the engine.
The old wasm32 double/shared-memory plugins cannot load in either new engine.
The SDK's generated `float-version.h` sets precision; do not pass
`-DUSE_DOUBLE=0` to the compiler, since the headers check whether it is defined.

## Use

Use the browser built from the upstream PR, or a later matching release:

```js
import { Csound } from "@csound/browser";
import { plugins } from "@csound/plugins";

const csound = await Csound({ withPlugins: [plugins.chua, plugins.image] });
```

To opt into wasm64 with double samples, change both imports:

```js
import { Csound } from "@csound/browser/wasm64.js";
import { plugins } from "@csound/plugins/wasm64";
```

Use `plugins.stk` to load the STK instruments. Each target's `stkops` WASM file
contains all bundled raw-wave samples. At initialization, the plugin writes
them to Csound's virtual filesystem and sets its wave path. No extra downloads
or `RAWWAVE_PATH` setup are needed. An explicit `RAWWAVE_PATH` overrides the
embedded samples; that directory must already contain the files STK needs.

The exports are asset URL strings. Bundlers must copy the WASM files referenced
by `new URL(..., import.meta.url)`. You can also serve files from
`@csound/plugins/dist/` yourself and pass their URLs to `withPlugins`.

To use `--opcode-lib`, write the plugin to Csound's virtual filesystem before
compiling your CSD. This does not need `withPlugins`:

```js
const csound = await Csound();
const response = await fetch(plugins.image);
if (!response.ok) throw new Error(`Plugin download failed: ${response.status}`);
await csound.fs.writeFile("/image.wasm", new Uint8Array(await response.arrayBuffer()));
await csound.setOption("--opcode-lib=/image.wasm");
await csound.compileCSD(csd);
```

You can also put the option in `<CsOptions>`. Its path refers to the virtual
filesystem, not a URL.

## Build and test

Install Nix and a Node.js version that meets [package.json](../package.json)'s
`engines` requirement, then run:

```sh
npm ci
npm run build
npm test
npm pack
```

Nix pins the toolchain and builds both CMake targets with Ninja.
Dependencies use static PIC builds for the matching pointer width. The engines
use unshared WASM memory; Preview 2 has no pthread spawning ABI. Web Audio's
shared transport buffers do not change the plugin memory type.
The C runtime imports the engine's allocator; it does not link its own heap.
STK uses a separate libc++ build with C++ exceptions
and a static WASM unwinder. Other C++ plugins keep exceptions disabled.
CMake's `STK_EMBED_RAWWAVES` option embeds the samples and defaults to on for WASM.
Native builds keep using the installed raw-wave directory by default.
The build uses `csound-plugin-sdk.tar.gz` for wasm32 and
`csound-wasm64-plugin-sdk.tar.gz` for wasm64. To override them, set
`CSOUND_PLUGIN_SDK` and `CSOUND_WASM64_PLUGIN_SDK` to absolute archive paths.
Set `CSOUND_WASM_PACKAGE` to the matching `@csound/wasm-bin` directory to also
override its toolchain and ABI metadata tools.
Set `NIX_SYSTEM=x86_64-linux` when using a Linux remote builder from macOS.

For each target, tests load all plugins together, render Chua and STK, compute a vector norm, and
save and reload a PNG through the WASI filesystem. STK tests also cover loading
it alone, overriding its wave path, and catching a missing-sample error.
Packing runs the tests and includes the WASM files and their license notices.
Each wasm64 plugin embeds its own Preview 2 adapter, so its WASI imports need
not be present in the engine. The private libc maps engine paths to its own
file descriptors while following the engine's working directory.
Tests use the upstream browser loader with each engine and check that it rejects
the other pointer width. `CSOUND_BROWSER_SOURCE` can select another browser
source directory; it defaults to `$CSOUND_WASM_PACKAGE/browser` for a source
checkout, or the installed `@csound/browser` package.

### Test against the upstream PR

Check out the commit pinned in the WASM workflow, then run from this repository:

```sh
export CSOUND_WASM_PACKAGE=/absolute/path/to/csound/platform/wasm-wasi
bash wasm/scripts/prepare-csound.sh
npm ci
npm run build
npm test
npm pack
```

The prepare step builds the two engines and SDK archives with Nix. It does not
download a WASI SDK bundle. Skip it when those artifacts already exist.
Before releasing, update the npm pins to versions containing #3556, remove the
source checkout from CI, and verify with the source overrides unset.

## Updating dependencies

Keep version pins in these files, rather than copying them into commands or docs:

- [package.json](../package.json) and [package-lock.json](../package-lock.json):
  update npm dependencies with `npm install --save-dev --save-exact <package>@<version>`.
  The Nix build reads the SDK version, URL, and hash from the lockfile.
- [nixpkgs.nix](nixpkgs.nix): the Nix toolchain revision and hash, shared by the
  plugin build and its static dependencies.

After an update, run the build, test, and pack commands above. The WASM workflow
uses the `csound` Cachix cache and uploads builds when `CACHIX_AUTH_TOKEN` is
available. Fork PRs read from the cache without the secret. Native builds use
GitHub Actions caches.

## npm releases

Native and npm packages share the version in `package.json`; see the
[release process](../README.md#releasing) for version changes and Git tags.
Run the build, test, and pack commands against matching published packages
with the source overrides unset. Publish a prerelease with
`npm publish --tag beta --access public`, or a stable release with
`npm publish --access public`, from that build. No workflow publishes to npm.

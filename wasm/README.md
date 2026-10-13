# @csound/plugins

WASM opcode plugins for the double-precision Csound 7 browser engine.

This first package includes Chua (`chuap`), image, and linear algebra. This build
disables STK, FluidSynth, HDF5, Python, WebSockets, device, and GPU plugins.
Csound 6 is not supported.

## Runtime dependency

This draft depends on [csound/csound#3416](https://github.com/csound/csound/pull/3416).
`@csound/browser@7.0.0-beta38` cannot load these plugins. CI builds the upstream
commit in that PR and tests against it. The package stays private until a browser
release includes those changes.

## Use

With a compatible browser build:

```js
import { Csound } from "@csound/browser";
import { plugins } from "@csound/plugins";

const csound = await Csound({ withPlugins: [plugins.chua, plugins.image] });
```

The exports are asset URL strings. Bundlers must copy the WASM files referenced
by `new URL(..., import.meta.url)`. You can also serve files from
`@csound/plugins/dist/` yourself and pass their URLs to `withPlugins`.

## Build and test

Install Nix and Node.js 22.13 or newer, then run:

```sh
npm ci
npm run build
```

Nix pins the toolchain and builds the existing `7.x` CMake targets with Ninja.
Dependencies use static PIC builds. The C runtime imports the engine's allocator;
it does not link its own heap. C++ plugins use libc++ without C++ exceptions.
The SDK comes from the locked `@csound/wasm-bin`
package. To use another Csound 7 SDK, set `CSOUND_PLUGIN_SDK` to an absolute archive
path. Set `NIX_SYSTEM=x86_64-linux` when using a Linux remote builder from macOS.

Until the upstream browser changes ship, check out their commit and build it
with the helper. It also supplies Java through Nix for the browser compiler:

```sh
git clone https://github.com/csound/csound.git .csound
git -C .csound checkout aed14beaa392dfe9c12270abd9cd63e57651be06
npm run build:runtime
export CSOUND_BROWSER_URL="file://$PWD/.csound/platform/wasm-wasi/browser/dist/csound.js"
npm test
npm pack
```

You can pass an existing Csound checkout to `npm run build:runtime -- /path/to/csound`.
`CSOUND_BROWSER_URL` also accepts the absolute file URL of a built browser bundle.
Without it, tests use the locked npm browser package. Tests load all plugins
together, render Chua, compute a vector norm, and save and reload a PNG through
the WASI filesystem. Packing runs the tests and includes the WASM files and their
license notices.

## First npm release

After the upstream fix ships, update the browser and WASM SDK pins and lockfile,
remove the temporary runtime checkout/build from CI, and run the checks against
the published browser package. Remove `private` from `package.json`, set the
package version, and run `npm publish --tag beta --access public` from a clean
build. No workflow publishes this package yet.

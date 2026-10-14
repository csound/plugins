# @csound/plugins

WASM opcode plugins for the double-precision Csound 7 browser engine.

This package includes Chua (`chuap`), image, linear algebra, and STK. This build
disables FluidSynth, HDF5, Python, WebSockets, device, and GPU plugins.
Csound 6 is not supported.

## Runtime dependency

Builds use the plugin SDK from `@csound/wasm-bin`, and tests use the published
`@csound/browser` runtime. [package.json](../package.json) and
[package-lock.json](../package-lock.json) record the versions used together.

## Use

Use a compatible `@csound/browser` release, such as the version pinned in
[package.json](../package.json):

```js
import { Csound } from "@csound/browser";
import { plugins } from "@csound/plugins";

const csound = await Csound({ withPlugins: [plugins.chua, plugins.image] });
```

Use `plugins.stk` to load the STK instruments. Its single `stkops.wasm` file
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

Nix pins the toolchain and builds the existing `7.x` CMake targets with Ninja.
Dependencies use static PIC builds. The C runtime imports the engine's allocator;
it does not link its own heap. STK uses a separate libc++ build with C++ exceptions
and a static WASM unwinder. Other C++ plugins keep exceptions disabled.
CMake's `STK_EMBED_RAWWAVES` option embeds the samples and defaults to on for WASM.
Native builds keep using the installed raw-wave directory by default.
To use another Csound 7 SDK, set `CSOUND_PLUGIN_SDK` to an absolute archive path.
Set `NIX_SYSTEM=x86_64-linux` when using a Linux remote builder from macOS.

Tests load all plugins together, render Chua and STK, compute a vector norm, and
save and reload a PNG through the WASI filesystem. STK tests also cover loading
it alone, overriding its wave path, and catching a missing-sample error.
Packing runs the tests and includes the WASM files and their license notices.
For work on the browser loader,
`CSOUND_BROWSER_URL` can point to an absolute file URL of a local browser bundle;
leave it unset to test the published package as CI does.

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

## First npm release

Run the build, test, and pack commands against the published browser package
with `CSOUND_BROWSER_URL` unset. When ready to publish, remove `private` from
`package.json`, set the package version with `npm version`, and run
`npm publish --tag beta --access public` from a clean build. No workflow publishes
this package yet.

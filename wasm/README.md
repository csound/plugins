# @csound/plugins

WASM opcode plugins for the double-precision Csound 7 browser engine.

This first package includes Chua (`chuap`), image, and linear algebra. This build
disables STK, FluidSynth, HDF5, Python, WebSockets, device, and GPU plugins.
Csound 6 is not supported.

## Runtime dependency

The engine and plugin SDK come from the released `@csound/wasm-bin` version in
[package-lock.json](../package-lock.json). The browser loader changes have
[merged upstream](https://github.com/csound/csound/pull/3416), but still need a
release of `@csound/browser`.

Until then, `npm run build:runtime` fetches the wrapper source pinned in
[runtime.json](runtime.json) and builds it with the released engine. CI uses the
same helper. The package stays private until the published browser can load it.

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
npm run build:runtime
export CSOUND_BROWSER_URL="file://$PWD/.csound/platform/wasm-wasi/browser/dist/csound.js"
npm test
npm pack
```

Nix pins the toolchain and builds the existing `7.x` CMake targets with Ninja.
Dependencies use static PIC builds. The C runtime imports the engine's allocator;
it does not link its own heap. C++ plugins use libc++ without C++ exceptions.
To use another Csound 7 SDK, set `CSOUND_PLUGIN_SDK` to an absolute archive path.
Set `NIX_SYSTEM=x86_64-linux` when using a Linux remote builder from macOS.

The runtime helper supplies Java through [shell.nix](shell.nix). It builds only
the JavaScript wrapper; it does not rebuild the engine. For work on the loader,
pass an existing Csound checkout to
`npm run build:runtime -- /path/to/csound` and point `CSOUND_BROWSER_URL` at that
checkout's built `platform/wasm-wasi/browser/dist/csound.js` file.

Without `CSOUND_BROWSER_URL`, tests use the locked npm browser package. Tests load
all plugins together, render Chua, compute a vector norm, and save and reload a
PNG through the WASI filesystem. Packing runs the tests and includes the WASM
files and their license notices.

## Updating dependencies

Keep version pins in these files, rather than copying them into commands or docs:

- [package.json](../package.json) and [package-lock.json](../package-lock.json):
  update npm dependencies with `npm install --save-dev --save-exact <package>@<version>`.
  The Nix build reads the SDK version, URL, and hash from the lockfile.
- [runtime.json](runtime.json): the temporary browser wrapper source revision.
  The helper fetches it for both local builds and CI.
- [nixpkgs.nix](nixpkgs.nix): the Nix toolchain revision and hash, shared by the
  plugin build and the wrapper build shell.

After an update, run the build, test, and pack commands above. The WASM workflow
uses the `csound` Cachix cache and uploads builds when `CACHIX_AUTH_TOKEN` is
available. Fork PRs read from the cache without the secret. Native builds use
GitHub Actions caches.

## First npm release

Once the browser loader ships, update `@csound/browser` and the lockfile, then
run the checks against that published package without `CSOUND_BROWSER_URL`.
Remove the temporary runtime build step, helper, source pin, and Java shell.
Remove `private` from `package.json`, set the package version, and run
`npm publish --tag beta --access public` from a clean build. No workflow publishes
this package yet.

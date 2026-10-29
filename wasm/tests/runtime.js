import { readFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

// Use the upstream loader with an explicit engine for each test target.
// Published browser entry points still select one engine per module.
globalThis.goog = { define: (_name, value) => value, global: globalThis };
const wasmRoot =
  process.env.CSOUND_WASM_PACKAGE ||
  dirname(fileURLToPath(import.meta.resolve("@csound/wasm-bin/package.json")));
const browserRoot =
  process.env.CSOUND_BROWSER_SOURCE ||
  (process.env.CSOUND_WASM_PACKAGE
    ? resolve(wasmRoot, "browser")
    : dirname(fileURLToPath(import.meta.resolve("@csound/browser/package.json"))));
const source = (name) => import(pathToFileURL(resolve(browserRoot, "src", name)));
const { default: loadWasm } = await source("module.js");
const bindings = Object.assign(
  {},
  ...(await Promise.all(
    ["instantiation", "performance", "attributes", "control-events", "rtaudio"].map((name) =>
      source(`modules/${name}.js`),
    ),
  )),
);
const { default: filesystem } = await source("filesystem/worker-fs.js");

export async function runtime(target, withPlugins = []) {
  const name = target === "wasm64" ? "csound-wasm64" : "csound";
  const [wasm, wasi] = await loadWasm({
    wasmDataURI: await readFile(resolve(wasmRoot, `lib/${name}.wasm.z`)),
    withPlugins,
    messagePort: {
      post: (message) => {
        if (message.log) console.log(message.log);
      },
    },
  });
  wasm.wasi = wasi;
  return {
    ...Object.fromEntries(
      Object.entries({ ...bindings, ...filesystem }).map(([name, bind]) => [name, bind(wasm)]),
    ),
    getMemory: () => wasm.exports.memory,
  };
}

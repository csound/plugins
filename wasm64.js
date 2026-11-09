/** Asset URLs for Csound 7's `withPlugins` option. */
export const plugins = Object.freeze({
  chua: new URL("./dist/chua-wasm64.wasm", import.meta.url).href,
  image: new URL("./dist/image-wasm64.wasm", import.meta.url).href,
  linearAlgebra: new URL("./dist/linear_algebra-wasm64.wasm", import.meta.url).href,
  stk: new URL("./dist/stkops-wasm64.wasm", import.meta.url).href,
});

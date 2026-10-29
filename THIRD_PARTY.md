# Third-party code in the WASM package

The plugins use the LGPL-2.1-or-later license in `LICENSE`. Their source and
copyright notices live under [`src`](https://github.com/csound/plugins/tree/develop/src).
The package also contains code from these libraries:

| Library | Used by | License notices |
| --- | --- | --- |
| Eigen | Chua | `dist/licenses/TARGET/eigen/` |
| GMM++ (GetFEM) | Linear algebra | `dist/licenses/TARGET/gmm.txt`; LGPL-3.0-or-later with the exception in the headers |
| libpng | Image | `dist/licenses/TARGET/libpng.txt` |
| zlib | Image | `dist/licenses/TARGET/zlib.txt` |
| STK and its raw-wave samples | STK instruments | `dist/licenses/TARGET/stk.txt` |
| wasi-libc, musl, cloudlibc, musl-fts | C runtime | `dist/licenses/TARGET/wasi-libc/` |
| LLVM libc++, libc++abi, libunwind, compiler-rt | C++ runtime and compiler support | `dist/licenses/TARGET/libcxx.txt`, `libcxxabi.txt`, `libunwind.txt`, `compiler-rt.txt` |

The Nix build copies these notices from the source used to build each library.
`TARGET` is `wasm32` or `wasm64`.
[`wasm/nixpkgs.nix`](https://github.com/csound/plugins/blob/develop/wasm/nixpkgs.nix)
pins their source versions. The GMM++ headers and STK sources and samples live
in this repository. The build uses static PIC libraries and a small CMake target
for LLVM's WASM unwinder. It patches wasi-libc's CMake file to omit allocator
objects and map paths into each plugin's own descriptor table. It also uses the
upstream Csound toolchain's wasm64 libc and compiler-rt
patches. `wasm/default.nix` and the selected Csound toolchain record those changes.

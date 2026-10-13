# Third-party code in the WASM package

The plugins use the LGPL-2.1-or-later license in `LICENSE`. Their source and
copyright notices live under [`7.x/src`](https://github.com/csound/plugins/tree/develop/7.x/src).
The package also contains code from these libraries:

| Library | Used by | License notices |
| --- | --- | --- |
| Eigen | Chua | `dist/licenses/eigen/` |
| GMM++ (GetFEM) | Linear algebra | `dist/licenses/gmm.txt`; LGPL-3.0-or-later with the exception in the headers |
| libpng | Image | `dist/licenses/libpng.txt` |
| zlib | Image | `dist/licenses/zlib.txt` |
| wasi-libc, musl, cloudlibc, musl-fts | C runtime | `dist/licenses/wasi-libc/` |
| LLVM libc++, libc++abi, compiler-rt | C++ runtime and compiler support | `dist/licenses/libcxx.txt`, `libcxxabi.txt`, `compiler-rt.txt` |

The Nix build copies these notices from the source used to build each library.
[`wasm/nixpkgs.nix`](https://github.com/csound/plugins/blob/develop/wasm/nixpkgs.nix)
pins their source versions. The GMM++ headers live in this repository. The build
uses upstream CMake and make options for static PIC libraries; it adds no source
patches. `wasm/default.nix` records those options.

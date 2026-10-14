{ system ? builtins.currentSystem
, pkgs ? import (import ./nixpkgs.nix) { inherit system; }
, csoundSdkArchive ? null
}:
let
  wasi = pkgs.pkgsCross.wasi32.clangStdenv;
  # Nix's WASI wrapper otherwise forces -fno-exceptions after CMake's flags.
  # Let each target select its exception mode instead.
  wasiCxx = pkgs.overrideCC wasi (wasi.cc.override (old: {
    nixSupport = old.nixSupport // {
      cc-cflags = builtins.filter (flag: flag != "-fno-exceptions") old.nixSupport.cc-cflags;
    };
  }));
  libc = pkgs.pkgsCross.wasi32.wasilibc.overrideAttrs (old: {
    NIX_CFLAGS_COMPILE = (old.NIX_CFLAGS_COMPILE or []) ++ [ "-fPIC" ];
    # Use the engine's allocator. wasi-libc also skips its fixed symbol list
    # for this mode; PIC adds imports absent from that non-PIC list.
    makeFlags = (old.makeFlags or []) ++ [ "MALLOC_IMPL=none" ];
    preFixup = "";
    dontStrip = true;
  });
  libcxx = pkgs.pkgsCross.wasi32.llvmPackages.libcxx.overrideAttrs (old: {
    cmakeFlags = old.cmakeFlags ++ [
      "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
      "-DLIBCXX_ENABLE_FILESYSTEM=ON"
    ];
    dontStrip = true;
  });
  libunwindSource = pkgs.pkgsCross.wasi32.llvmPackages.libunwind.src;
  libunwind = wasi.mkDerivation {
    pname = "libunwind-wasm-pic";
    inherit (libcxx) version;
    src = ./libunwind;
    nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
    cmakeFlags = [ "-DLIBUNWIND_SOURCE_DIR=${libunwindSource}/libunwind" ];
    dontStrip = true;
  };
  libcxxExceptions = (libcxx.override { stdenv = wasiCxx; }).overrideAttrs (old: {
    pname = "libcxx-wasm-exceptions";
    cmakeFlags = old.cmakeFlags ++ [
      "-DLIBCXX_ENABLE_EXCEPTIONS=ON"
      "-DLIBCXX_ADDITIONAL_LIBRARIES="
      "-DLIBCXXABI_ENABLE_EXCEPTIONS=ON"
    ];
    preConfigure = (old.preConfigure or "") + ''
      cmakeFlagsArray+=("-DCMAKE_CXX_FLAGS=-fexceptions -fwasm-exceptions -I${libunwindSource}/libunwind/include")
    '';
  });
  lock = builtins.fromJSON (builtins.readFile ../package-lock.json);
  wasmBin = lock.packages."node_modules/@csound/wasm-bin";
  sdkArchive = if csoundSdkArchive == null then null else builtins.path {
    path = csoundSdkArchive;
    name = "csound-plugin-sdk.tar.gz";
  };
  sdk = pkgs.runCommand "csound-wasm-plugin-sdk-${wasmBin.version}" {} ''
    mkdir -p $out
    ${if csoundSdkArchive == null then ''
      tar -xzf ${pkgs.fetchurl { url = wasmBin.resolved; hash = wasmBin.integrity; }}
      tar -xzf package/lib/csound-plugin-sdk.tar.gz --strip-components=1 -C $out
    '' else ''
      tar -xzf ${sdkArchive} --strip-components=1 -C $out
    ''}
  '';
  zlib = wasi.mkDerivation {
    pname = "zlib-wasm-pic";
    inherit (pkgs.zlib) version src;
    nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
    cmakeFlags = [ "-DCMAKE_POSITION_INDEPENDENT_CODE=ON" "-DCMAKE_INSTALL_LIBDIR=lib" "-DZLIB_BUILD_EXAMPLES=OFF" ];
    dontStrip = true;
  };
  libpng = wasi.mkDerivation {
    pname = "libpng-wasm-pic";
    inherit (pkgs.libpng) version src;
    nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
    buildInputs = [ zlib ];
    cmakeFlags = [
      "-DUNIX=ON"
      "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
      "-DCMAKE_INSTALL_LIBDIR=lib"
      "-DPNG_SHARED=OFF" "-DPNG_STATIC=ON" "-DPNG_TESTS=OFF" "-DPNG_TOOLS=OFF"
      "-DZLIB_LIBRARY=${zlib}/lib/libzlibstatic.a"
    ];
    NIX_CFLAGS_COMPILE = [ "-mllvm" "-wasm-enable-sjlj" ];
    dontStrip = true;
  };
in wasiCxx.mkDerivation {
  pname = "csound-plugins-wasm";
  version = (builtins.fromJSON (builtins.readFile ../package.json)).version;
  src = pkgs.lib.cleanSource ../7.x;
  nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
  buildInputs = [ libpng zlib ];
  dontStrip = true;
  cmakeFlags = [
    "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCSOUND_INCLUDE_DIR=${sdk}/include/csound"
    "-DWASM_LIBC=${libc}/lib/libc.a"
    "-DWASM_SETJMP=${libc}/lib/libsetjmp.a"
    "-DWASM_LIBCXX=${libcxx}/lib/libc++.a"
    "-DWASM_CXX_INCLUDE_DIR=${libcxx.dev}/include/c++/v1"
    "-DWASM_LIBCXX_EXCEPTIONS=${libcxxExceptions}/lib/libc++.a"
    "-DWASM_CXX_EXCEPTIONS_INCLUDE_DIR=${libcxxExceptions.dev}/include/c++/v1"
    "-DWASM_LIBUNWIND=${libunwind}/lib/libunwind.a"
    "-DEIGEN3_INCLUDE_DIR=${pkgs.eigen}/include/eigen3"
    "-DZLIB_LIBRARY=${zlib}/lib/libzlibstatic.a"
    "-DPNG_LIBRARY=${libpng}/lib/libpng.a"
    "-DBUILD_CHUA_OPCODES=ON"
    "-DBUILD_IMAGE_OPCODES=ON"
    "-DBUILD_LINEAR_ALGEBRA_OPCODES=ON"
    "-DBUILD_STK_OPCODES=ON"
    "-DBUILD_FLUID_OPCODES=OFF"
    "-DBUILD_HDF5_OPCODES=OFF"
    "-DBUILD_WEBSOCKET_OPCODE=OFF"
    "-DBUILD_PYTHON_OPCODES=OFF"
    "-DBUILD_WIIMOTE_OPCODES=OFF"
    "-DBUILD_P5GLOVE_OPCODES=OFF"
    "-DBUILD_CUDA_OPCODES=OFF"
    "-DBUILD_OPENCL_OPCODES=OFF"
  ];
  postInstall = ''
    mkdir -p $out/share/licenses/wasi-libc $out/share/licenses/eigen
    cp ${libc.src}/LICENSE* $out/share/licenses/wasi-libc/
    cp ${libc.src}/libc-top-half/musl/COPYRIGHT $out/share/licenses/wasi-libc/musl.txt
    cp ${libc.src}/libc-bottom-half/cloudlibc/LICENSE $out/share/licenses/wasi-libc/cloudlibc.txt
    cp ${libc.src}/fts/musl-fts/COPYING $out/share/licenses/wasi-libc/musl-fts.txt
    cp ${libcxx.src}/libcxx/LICENSE.TXT $out/share/licenses/libcxx.txt
    cp ${libcxx.src}/libcxxabi/LICENSE.TXT $out/share/licenses/libcxxabi.txt
    cp ${libunwindSource}/libunwind/LICENSE.TXT $out/share/licenses/libunwind.txt
    cp ${../7.x/src/stk/LICENSE} $out/share/licenses/stk.txt
    cp ${pkgs.pkgsCross.wasi32.llvmPackages.compiler-rt.src}/compiler-rt/LICENSE.TXT $out/share/licenses/compiler-rt.txt
    cp ${pkgs.eigen.src}/COPYING* $out/share/licenses/eigen/
    sed -n '1,/^=*[*][/]$/p' ${../7.x/src/linear_algebra/gmm/gmm.h} > $out/share/licenses/gmm.txt
    tar -xOf ${pkgs.libpng.src} libpng-${pkgs.libpng.version}/LICENSE > $out/share/licenses/libpng.txt
    tar -xOf ${pkgs.zlib.src} zlib-${pkgs.zlib.version}/LICENSE > $out/share/licenses/zlib.txt
    for plugin in chua image linear_algebra stkops; do
      test -s "$out/lib/$plugin.wasm"
    done
  '';
}

{ system ? builtins.currentSystem
, pkgs ? import (import ./nixpkgs.nix) { inherit system; }
, wasm64 ? true
, csoundWasmPackage ? null
, csoundSdkArchive ? null
}:
let
  lock = builtins.fromJSON (builtins.readFile ../package-lock.json);
  wasmBin = lock.packages."node_modules/@csound/wasm-bin";
  publishedPackage = pkgs.runCommand "csound-wasm-bin-${wasmBin.version}" {} ''
    mkdir -p $out
    tar -xzf ${pkgs.fetchurl { url = wasmBin.resolved; hash = wasmBin.integrity; }} --strip-components=1 -C $out
  '';
  wasmPackage = if csoundWasmPackage == null then publishedPackage else
    pkgs.lib.cleanSourceWith {
      src = builtins.toPath csoundWasmPackage;
      filter = path: type:
        let
          root = toString (builtins.toPath csoundWasmPackage);
          relative = pkgs.lib.removePrefix (root + "/") path;
        in path == root || builtins.elem relative [ "src" "scripts" "scripts/wasm64" "lib" ]
          || pkgs.lib.hasPrefix "src/" relative
          || (pkgs.lib.hasPrefix "scripts/wasm64/" relative && pkgs.lib.hasSuffix ".py" relative)
          || builtins.elem relative [
            "lib/csound-plugin-sdk.tar.gz" "lib/csound-wasm64-plugin-sdk.tar.gz"
          ];
    };
  toolchainFile = if wasm64 then "wasi64-toolchain.nix" else "wasi-preview2-toolchain.nix";
  toolchain = if builtins.pathExists "${wasmPackage}/src/${toolchainFile}"
    then import "${wasmPackage}/src/${toolchainFile}" { inherit system pkgs; }
    else throw "Preview 2 plugins require csound/csound#3556; set CSOUND_WASM_PACKAGE to its built platform/wasm-wasi directory until a matching @csound/wasm-bin is published.";
  inherit (toolchain) pkgsWasm;
  suffix = pkgs.lib.optionalString wasm64 "-wasm64";
  wasi = toolchain.stdenv;
  # Nix's WASI wrapper otherwise forces -fno-exceptions after CMake's flags.
  # Let each target select its exception mode instead.
  wasiCxx = pkgs.overrideCC wasi (wasi.cc.override (old: {
    nixSupport = old.nixSupport // {
      cc-cflags = builtins.filter (flag: flag != "-fno-exceptions") old.nixSupport.cc-cflags;
    };
  }));
  libc = toolchain.libc.overrideAttrs (old: {
    nativeBuildInputs = old.nativeBuildInputs ++ [ pkgs.python3 ];
    NIX_CFLAGS_COMPILE = (old.NIX_CFLAGS_COMPILE or []) ++ [
      "-fPIC" "-mllvm" "-wasm-enable-sjlj" "-mllvm" "-wasm-use-legacy-eh=false"
    ];
    # Use the engine's allocator. wasi-libc 32's CMake build still references
    # empty malloc targets in MALLOC=none mode, so omit those object lists.
    postPatch = (old.postPatch or "") + ''
      python3 ${./scripts/patch-libc.py}
      substituteInPlace CMakeLists.txt \
        --replace-fail '$<TARGET_OBJECTS:''${malloc_target}-shared>' "" \
        --replace-fail '$<TARGET_OBJECTS:''${malloc_target}-static>' ""
    '';
    cmakeFlags = old.cmakeFlags ++ [ "-DMALLOC=none" "-DCMAKE_POSITION_INDEPENDENT_CODE=ON" ];
    preFixup = "";
    dontStrip = true;
  });
  libcxx = (pkgsWasm.llvmPackages.libcxx.override { stdenv = wasiCxx; }).overrideAttrs (old: {
    cmakeFlags = old.cmakeFlags ++ [
      "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
      "-DLIBCXX_ENABLE_FILESYSTEM=ON"
      "-DLIBCXX_ADDITIONAL_LIBRARIES="
    ];
    dontStrip = true;
  });
  libunwindSource = pkgsWasm.llvmPackages.libunwind.src;
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
      cmakeFlagsArray+=("-DCMAKE_CXX_FLAGS=-fexceptions -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false -I${libunwindSource}/libunwind/include")
    '';
  });
  sdkArchive = if csoundSdkArchive == null then null else builtins.path {
    path = csoundSdkArchive;
    name = "csound-plugin-sdk.tar.gz";
  };
  sdk = pkgs.runCommand "csound-wasm-plugin-sdk-${wasmBin.version}" {} ''
    mkdir -p $out
    ${if csoundSdkArchive == null then ''
      tar -xzf ${wasmPackage}/lib/csound${suffix}-plugin-sdk.tar.gz --strip-components=1 -C $out
    '' else ''
      tar -xzf ${sdkArchive} --strip-components=1 -C $out
    ''}
  '';
  zlib = wasi.mkDerivation {
    pname = "zlib-wasm-pic";
    inherit (pkgs.zlib) version src;
    nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
    cmakeFlags = [ "-DCMAKE_POSITION_INDEPENDENT_CODE=ON" "-DCMAKE_INSTALL_LIBDIR=lib" "-DZLIB_BUILD_EXAMPLES=OFF" ];
    # Build only
    # the static target so it does not link the toolchain's non-PIC libc.
    ninjaFlags = [ "zlibstatic" ];
    installPhase = ''
      runHook preInstall
      install -Dm644 libz.a $out/lib/libz.a
      install -Dm644 ../zlib.h $out/include/zlib.h
      install -Dm644 zconf.h $out/include/zconf.h
      runHook postInstall
    '';
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
      "-DZLIB_LIBRARY=${zlib}/lib/libz.a"
    ];
    NIX_CFLAGS_COMPILE = [ "-mllvm" "-wasm-enable-sjlj" "-mllvm" "-wasm-use-legacy-eh=false" ];
    dontStrip = true;
  };
in wasiCxx.mkDerivation {
  pname = "csound-plugins-${if wasm64 then "wasm64" else "wasm32"}-preview2";
  version = (builtins.fromJSON (builtins.readFile ../package.json)).version;
  src = pkgs.lib.sourceByRegex ../. [
    "CMakeLists\\.txt"
    "cmake"
    "cmake/.*"
    "src"
    "src/.*"
    "tests"
    "tests/.*"
  ];
  nativeBuildInputs = [ pkgs.cmake pkgs.ninja pkgs.python3 pkgs.wasm-tools ];
  buildInputs = [ libpng zlib ];
  dontStrip = true;
  cmakeFlags = [
    "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCSOUND_INCLUDE_DIR=${sdk}/include/csound"
    "-DUSE_DOUBLE=${if wasm64 then "ON" else "OFF"}"
    "-DWASM_LIBC=${libc}/lib/${toolchain.target}/libc.a"
    "-DWASM_SETJMP=${libc}/lib/${toolchain.target}/libsetjmp.a"
    "-DWASM_LIBCXX=${libcxx}/lib/libc++.a"
    "-DWASM_CXX_INCLUDE_DIR=${libcxx.dev}/include/c++/v1"
    "-DWASM_LIBCXX_EXCEPTIONS=${libcxxExceptions}/lib/libc++.a"
    "-DWASM_CXX_EXCEPTIONS_INCLUDE_DIR=${libcxxExceptions.dev}/include/c++/v1"
    "-DWASM_LIBUNWIND=${libunwind}/lib/libunwind.a"
    "-DEIGEN3_INCLUDE_DIR=${pkgs.eigen}/include/eigen3"
    "-DZLIB_LIBRARY=${zlib}/lib/libz.a"
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
    cp ${../src/stk/LICENSE} $out/share/licenses/stk.txt
    cp ${pkgsWasm.llvmPackages.compiler-rt.src}/compiler-rt/LICENSE.TXT $out/share/licenses/compiler-rt.txt
    cp ${pkgs.eigen.src}/COPYING* $out/share/licenses/eigen/
    sed -n '1,/^=*[*][/]$/p' ${../src/linear_algebra/gmm/gmm.h} > $out/share/licenses/gmm.txt
    tar -xOf ${pkgs.libpng.src} libpng-${pkgs.libpng.version}/LICENSE > $out/share/licenses/libpng.txt
    tar -xOf ${pkgs.zlib.src} zlib-${pkgs.zlib.version}/LICENSE > $out/share/licenses/zlib.txt
    for plugin in chua image linear_algebra stkops; do
      test -s "$out/lib/$plugin.wasm"
      ${pkgs.lib.optionalString wasm64 ''
        python3 ${./scripts/write-abi.py} ${wasmPackage}/scripts/wasm64 "$out/lib/$plugin.wasm" "$out/lib/$plugin-wasm64.wasm"
        rm "$out/lib/$plugin.wasm"
      ''}
    done
  '';
}

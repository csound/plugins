#!/usr/bin/env bash
# Build only the two reactors and SDKs needed by this package.
set -euo pipefail
: "${CSOUND_WASM_PACKAGE:?Set CSOUND_WASM_PACKAGE to the platform/wasm-wasi directory in a Csound checkout}"
cd "$CSOUND_WASM_PACKAGE"
source scripts/nixpkgs-pin.sh
mkdir -p lib
for target in wasm32 wasm64; do
  source=src/csound.nix
  name=csound
  if [[ "$target" == wasm64 ]]; then
    source=src/csound-wasm64.nix
    name=csound-wasm64
  fi
  nix-build "$source" --argstr moduleKind browser -o "result-plugin-$target"
  for file in "$name.wasm" "$name.wasm.z" "$name-plugin-sdk.tar.gz"; do
    cp "result-plugin-$target/lib/$file" "lib/$file"
  done
done

#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."

args=(--show-trace)
if [[ -n "${NIX_SYSTEM:-}" ]]; then
  args+=(--argstr system "$NIX_SYSTEM")
fi
if [[ -n "${CSOUND_WASM_PACKAGE:-}" ]]; then
  args+=(--argstr csoundWasmPackage "$CSOUND_WASM_PACKAGE")
fi
staging=$(mktemp -d .wasm-dist.XXXXXX)
trap 'rm -rf "$staging"' EXIT
for target in wasm32 wasm64; do
  wide=false
  suffix=""
  sdk="${CSOUND_PLUGIN_SDK:-}"
  if [[ "$target" == wasm64 ]]; then
    wide=true
    suffix="-wasm64"
    sdk="${CSOUND_WASM64_PLUGIN_SDK:-}"
  fi
  target_args=("${args[@]}" --arg wasm64 "$wide" -o "result-$target")
  if [[ -n "$sdk" ]]; then
    target_args+=(--argstr csoundSdkArchive "$sdk")
  fi
  nix-build wasm/default.nix "${target_args[@]}"
  for plugin in chua image linear_algebra stkops; do
    install -m 644 "result-$target/lib/$plugin$suffix.wasm" "$staging/$plugin$suffix.wasm"
  done
  mkdir -p "$staging/licenses/$target"
  cp -R "result-$target/share/licenses/." "$staging/licenses/$target/"
done
chmod -R u+w "$staging/licenses"
if [[ -d dist ]]; then
  chmod -R u+w dist
fi
rm -rf dist
mv "$staging" dist

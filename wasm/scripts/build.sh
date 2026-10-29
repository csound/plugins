#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."

args=(--show-trace -o result-wasm)
if [[ -n "${NIX_SYSTEM:-}" ]]; then
  args+=(--argstr system "$NIX_SYSTEM")
fi
if [[ -n "${CSOUND_PLUGIN_SDK:-}" ]]; then
  args+=(--argstr csoundSdkArchive "$CSOUND_PLUGIN_SDK")
fi
nix-build wasm/default.nix "${args[@]}"
staging=$(mktemp -d .wasm-dist.XXXXXX)
trap 'rm -rf "$staging"' EXIT
for plugin in chua image linear_algebra stkops; do
  install -m 644 "result-wasm/lib/$plugin.wasm" "$staging/$plugin.wasm"
done
cp -R result-wasm/share/licenses "$staging/licenses"
chmod -R u+w "$staging/licenses"
if [[ -d dist ]]; then
  chmod -R u+w dist
fi
rm -rf dist
mv "$staging" dist

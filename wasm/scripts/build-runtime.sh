#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."

# Temporary until the loader changes ship in @csound/browser. Use the released
# engine from this project's lockfile; only the JavaScript wrapper needs a build.
wasm_bin_dir="$PWD/node_modules/@csound/wasm-bin/lib"
if [[ $# -eq 0 ]]; then
  revision=$(node -p 'require("./wasm/runtime.json").revision')
  csound_source="$PWD/.csound"
  if [[ ! -d "$csound_source" ]]; then
    git init --quiet "$csound_source"
  fi
  if [[ "$(git -C "$csound_source" rev-parse HEAD 2>/dev/null || true)" != "$revision" ]]; then
    git -C "$csound_source" fetch --depth 1 https://github.com/csound/csound.git "$revision"
    git -C "$csound_source" checkout --detach "$revision"
  fi
else
  csound_source=$(cd "$1" && pwd)
fi

browser_dir="$csound_source/platform/wasm-wasi/browser"
npm ci --prefix "$browser_dir"
for file in csound.wasm csound.wasm.z; do
  install -m 644 "$wasm_bin_dir/$file" "$browser_dir/node_modules/@csound/wasm-bin/lib/$file"
done
export CSOUND_BROWSER_BUILD_DIR="$browser_dir"
# Expand the exported path in the shell that Nix starts.
# shellcheck disable=SC2016
nix-shell wasm/shell.nix --run \
  'npm run build:prod --prefix "$CSOUND_BROWSER_BUILD_DIR"'

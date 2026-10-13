#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."

# Temporary until the runtime changes ship in @csound/browser and @csound/wasm-bin.
csound_source=$(cd "${1:-.csound}" && pwd)
args=(-o result-browser --argstr moduleKind browser)
if [[ -n "${NIX_SYSTEM:-}" ]]; then
  args+=(--argstr system "$NIX_SYSTEM")
fi
nixpkgs=$(nix-instantiate --eval --strict --json wasm/nixpkgs.nix | node -e \
  'let input=""; process.stdin.on("data", c => input += c); process.stdin.on("end", () => console.log(JSON.parse(input)));')
nix-build "$csound_source/platform/wasm-wasi/src/csound.nix" -I "nixpkgs=$nixpkgs" "${args[@]}"

browser_dir="$csound_source/platform/wasm-wasi/browser"
npm ci --prefix "$browser_dir"
for file in csound.wasm csound.wasm.z; do
  install -m 644 "result-browser/lib/$file" "$browser_dir/node_modules/@csound/wasm-bin/lib/$file"
done
export CSOUND_BROWSER_BUILD_DIR="$browser_dir"
# Expand the exported path in the shell that Nix starts.
# shellcheck disable=SC2016
nix-shell -I "nixpkgs=$nixpkgs" -p jdk21 --run \
  'npm run build:prod --prefix "$CSOUND_BROWSER_BUILD_DIR"'

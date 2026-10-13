# Keep in sync with csound/csound's platform/wasm-wasi/scripts/nixpkgs-pin.sh.
builtins.fetchTarball {
  url = "https://github.com/NixOS/nixpkgs/archive/4db2c220f32fd162658ed1b7bb2f46a82996ddbe.tar.gz";
  sha256 = "08b1k0zrbal2if94bph2ap9f73zlplsh05a65jbdkp5ipycmg92s";
}

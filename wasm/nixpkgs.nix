# Match csound/csound#3556 (LLVM 23; Csound pins the Preview 2 libc source).
builtins.fetchTarball {
  url = "https://github.com/NixOS/nixpkgs/archive/6dadaf4375be35033e1c84395adbd53ff5fded96.tar.gz";
  sha256 = "1g9sadx7zszll972hlv78675vc5vz2iw8v9jzs4338ccx8gzp48g";
}

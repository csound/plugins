# Match csound/csound#3556 (LLVM 21 and wasi-libc 32).
builtins.fetchTarball {
  url = "https://github.com/NixOS/nixpkgs/archive/d3b13ad75cc492690728f9307e443dec9ef9a20e.tar.gz";
  sha256 = "05r2yhqg472368kf2i0mqrvbfww94ykssphkxdxxgjxwd1jffqx4";
}

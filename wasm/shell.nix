{ pkgs ? import (import ./nixpkgs.nix) {} }:
pkgs.mkShell {
  packages = [ pkgs.jdk21 ];
}

CSOUND PLUGINS
===

This repository contains the code for Csound plugins that require
external dependencies, as well as code for new third-party contributed
plugins. 

The tree is currently organised by major Csound versions. Full and beta releases are available and build artefacts are also accessible from the actions tab.

The Csound 7 WASM package, `@csound/plugins`, uses Nix and CMake. See the
[build, test, and npm release notes](wasm/README.md).

NB: Csound 6.x is EOL. It is likely that plugins for that version will be removed from the develop and main branch after the first full release. In that case, we will keep the code solely in the csound6 branch.




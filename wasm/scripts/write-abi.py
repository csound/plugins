"""Give wasm64 plugins their ABI metadata and Preview 2 adapter."""
import sys

sys.path.insert(0, sys.argv[1])
from wasm import VALUES
from bridge import build

# LLVM's standardized exception runtime exposes exnref in its signatures.
# It stays an opaque reference; only i64 values need JS/BigInt conversion.
VALUES[0x69] = "exnref"
build(sys.argv[2], sys.argv[3], browser=True)

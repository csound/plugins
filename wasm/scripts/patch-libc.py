"""Keep Preview 2 file descriptors inside the plugin's private libc."""
from pathlib import Path

header = Path("libc-bottom-half/headers/public/wasi/libc-find-relpath.h")
text = header.read_text()
marker = "#ifdef __cplusplus\n}\n#endif"
assert text.count(marker) == 1
text = text.replace(marker, """
/* Resolve the engine's cwd, then map its preopen prefix to our descriptor
 * table. A Preview 2 libc fd is an index into that libc's private table;
 * copying the engine's fd skips initialization and can select another file. */
static inline int csound_plugin_find_relpath_alloc(
    const char *path, const char **abs, char **relative,
    size_t *relative_len, int can_realloc) {
  if (__wasilibc_find_relpath_alloc(path, abs, relative, relative_len,
                                   can_realloc) < 0)
    return -1;
  const char *local_prefix;
  const char *local_relative;
  return __wasilibc_find_abspath(*abs, &local_prefix, &local_relative);
}

""" + marker)
header.write_text(text)
for name in ("posix.c", "preopens.c"):
    path = Path("libc-bottom-half/sources") / name
    text = path.read_text()
    old = "return __wasilibc_find_relpath_alloc("
    assert text.count(old) == 1, name
    path.write_text(text.replace(old, "return csound_plugin_find_relpath_alloc("))

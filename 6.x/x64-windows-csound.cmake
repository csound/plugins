set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)

if(${PORT} MATCHES "fluidsynth|glib|zlib|pcre|libffi|gettext|libiconv|portmidi|portaudio|liblo")
	set(VCPKG_LIBRARY_LINKAGE dynamic)
else()
	set(VCPKG_LIBRARY_LINKAGE static)
endif()

# Allow PKG_CONFIG (set in CI to a chocolatey-installed pkg-config) through to
# port builds. Without this, vcpkg builds ports in a clean environment and
# vcpkg_find_acquire_program(PKGCONFIG) ignores ENV{PKG_CONFIG}, falling back
# to downloading its own pkgconf from MSYS2 mirrors, which can 404.
set(VCPKG_ENV_PASSTHROUGH PKG_CONFIG)

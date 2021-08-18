#Simple script to build plugins on Windows. Modify each of the paths below for your own needs.. 

cd build
cmake -DFLTK_BASE_LIBRARY_DEBUG=../../../../vcpkg/installed/x64-windows-csound/lib/fltkd.lib \
-DFLTK_BASE_LIBRARY_RELEASE=../../vcpkg/installed/x64-windows-csound/lib/fltk.lib \
-DFLTK_INCLUDE_DIR=../../vcpkg/packages/fltk_x64-windows-csound/include \
-DFLTK_FLUID_EXECUTABLE=../../vcpkg/installed/x64-windows-csound/tools/fluidsynth/fluidsynth.exe \
-DFLTK_FORMS_LIBRARY_RELEASE=../../vcpkg/installed/x64-windows-csound/lib/fltk_forms.lib \
-DFLTK_IMAGES_LIBRARY_RELEASE=../../vcpkg/installed/x64-windows-csound/lib/fltk_images.lib \
-DABLETON_LINK_HOME=D:/sourcecode/link \
-DPNG_PNG_INCLUDE_DIR=../../vcpkg/installed/x64-windows-csound/include \
-DPNG_LIBRARY_RELEASE=C:/Python36/DLLs/png.lib \
-DEIGEN3_INCLUDE_DIR=D:/sourcecode/eigen-3.4-rc1 \
-DFAUST_INCLUDE_DIR_HINT="C:/Program Files/Faust/include" \
-DFAUST_LIB_DIR_HINT="C:/Program Files/Faust/lib" \
-DBUILD_FAUST_OPCODES=ON ..

cmake --build . --config Release




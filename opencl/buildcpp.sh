set -x
c++ -O3 -dynamiclib -o libclopp.dylib -DUSE_DOUBLE -D_FORTIFY_SOURCE=0 cladsynth2.cpp -I../../include -framework OpenCL -Wno-deprecated-declarations -std=c++11

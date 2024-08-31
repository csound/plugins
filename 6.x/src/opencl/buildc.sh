set -x
c++ -g -O3 -dynamiclib -o libclconv.dylib -std=c++11 -DUSE_DOUBLE -D_FORTIFY_SOURCE=0 clconv.cpp -I../../include -framework OpenCL -Wno-deprecated-declarations

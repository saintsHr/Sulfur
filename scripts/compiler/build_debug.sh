cmake -S ./compiler -B ./build/compiler -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=TRUE
cmake --build ./build/compiler

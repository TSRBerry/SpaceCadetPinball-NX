#!/bin/sh

if [ -d ./build ]; then
    rm -rf ./build
fi

mkdir -p ./build

cd ./build

cmake -DCMAKE_TOOLCHAIN_FILE="${DEVKITPRO}/cmake/Switch.cmake" ..

make -j$(nproc)

cd ..

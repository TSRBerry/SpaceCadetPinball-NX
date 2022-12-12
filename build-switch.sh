#!/bin/sh

if [ -d ./build ]; then
    rm -rf ./build
fi

mkdir -p ./build ./out

INSTALL_DIR="${PWD}/out"

cd ./build

cmake -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" -DCMAKE_TOOLCHAIN_FILE="${DEVKITPRO}/cmake/Switch.cmake" ..

make -j$(nproc)

make install

cd ..

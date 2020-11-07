#! /usr/bin/env bash

if [[ ! -d build ]]; then
    mkdir build
fi

cd build || exit 1
if [[ -n "$(ls -A .)" ]]; then
    rm -r ./*
fi

cmake ..
cmake --build . --parallel 8

cd ..

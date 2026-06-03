#!/bin/sh

# Ensure submodules are initialized
git submodule update --init --recursive

# CMake generates version header from git
cmake -B build -S . -G "Ninja" \
	-DCMAKE_BUILD_TYPE=Release

cmake --build build --config Release

#!/bin/sh

./version_from_git.sh

# Ensure submodules are initialized
git submodule update --init --recursive

cmake -B build -S . -G "Ninja" \
	-DCMAKE_BUILD_TYPE=Release

cmake --build build --config Release

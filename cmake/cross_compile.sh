#!/bin/bash
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/WindowsToolchain.cmake -S .. -B ../builds/cross-windows

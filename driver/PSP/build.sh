#!/bin/sh
psp-cmake ${CMAKE_ARGS} ../../
make -j"$(nproc)"
make install

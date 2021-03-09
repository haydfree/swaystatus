#!/bin/bash -ex

CPUS=$(nproc)

BUILD_DIR=debug_build/ DEBUG=true make -j $CPUS
BUILD_DIR=release_build/ make -j $CPUS
BUILD_DIR=release_no_py_build/ PYTHON=false exec make -j $CPUS

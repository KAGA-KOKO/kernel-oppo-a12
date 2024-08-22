#!/bin/bash

# set environment variables
git clone --depth=1 https://github.com/KAGA-KOKO/clang-6.0.2 clang
export KBUILD_BUILD_HOST="PIE"
export KBUILD_BUILD_USER="KAGA-KOKO"
export PATH="$(pwd)/clang/bin:$PATH"
export LOCALVERSION="-P-OSS"

# build kernel
make -j$(nproc --all) O=out ARCH=arm64 oppo6765_19451_defconfig
make -j16 ARCH=arm64 O=out \
                      HOSTCC=clang \
                      HOSTCXX=clang++ \
                      CC=clang \
                      CROSS_COMPILE=aarch64-linux-gnu- \
                      CROSS_COMPILE_ARM32=arm-linux-gnueabi-

#!/bin/bash

LLVM_VERSION=$1

if [[ -z "`apt-key list | grep -i llvm`" ]]; then
    # download GPG key once
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
fi

add-apt-repository 'deb http://apt.llvm.org/jammy/  llvm-toolchain-jammy main'
apt-get update
apt-get install -y clang-$LLVM_VERSION lld-$LLVM_VERSION clang-tidy-$LLVM_VERSION clang-format-$LLVM_VERSION llvm-$LLVM_VERSION-dev lld-$LLVM_VERSION libc++-$LLVM_VERSION-dev libc++abi-$LLVM_VERSION-dev libunwind-$LLVM_VERSION-dev

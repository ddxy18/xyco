#!/bin/bash

LLVM_VERSION=$1

# ninja
wget -q https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-linux.zip
unzip ninja-linux.zip -d /usr/bin/

# cmake
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy-rc main' | tee -a /etc/apt/sources.list.d/kitware.list >/dev/null

# llvm
if [[ ! -f /etc/apt/trusted.gpg.d/apt.llvm.org.asc ]]; then
    # download GPG key once
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc
fi
add-apt-repository "deb http://apt.llvm.org/jammy/  llvm-toolchain-jammy-$LLVM_VERSION main"

apt-get update
# FIXME(xiaoyu): In conflict with default libc++ packages in github runner, so remove them first.
apt-get remove -y 'libc++1-*' 'libc++abi1-*' 'libunwind-*'
apt-get install -y cmake clang-$LLVM_VERSION lld-$LLVM_VERSION clang-tidy-$LLVM_VERSION clang-format-$LLVM_VERSION llvm-$LLVM_VERSION-dev lld-$LLVM_VERSION libc++-$LLVM_VERSION-dev libc++abi-$LLVM_VERSION-dev libunwind-$LLVM_VERSION-dev

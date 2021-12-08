# syntax=docker/dockerfile:1
FROM ubuntu:21.10

ENV llvm_version=13

# install essential packages
RUN ln -snf /usr/share/zoneinfo/$CONTAINER_TIMEZONE /etc/localtime && echo $CONTAINER_TIMEZONE > /etc/timezone \
    && sed -i 's/http:\/\/archive.ubuntu.com/http:\/\/mirror.sjtu.edu.cn/g' /etc/apt/sources.list \
    && apt-get -y update && apt-get -y upgrade \
    && apt-get install -y vim zsh curl pkg-config libssl-dev git cmake binutils libstdc++-11-dev \
    clang-$llvm_version lldb-$llvm_version clang-format-$llvm_version clang-tidy-$llvm_version clangd-$llvm_version \
    libc++-$llvm_version-dev libc++abi-$llvm_version-dev libunwind-$llvm_version-dev \
    && ln -s /usr/bin/clang-$llvm_version /usr/bin/clang && ln -s /usr/bin/clang++-$llvm_version /usr/bin/clang++ \
    && ln -s /usr/bin/lldb-$llvm_version /usr/bin/lldb \
    && ln -s /usr/bin/clang-format-$llvm_version /usr/bin/clang-format \
    && ln -s /usr/bin/clang-tidy-$llvm_version /usr/bin/clang-tidy && ln -s /usr/bin/clangd-$llvm_version /usr/bin/clangd \
    && git config --global core.editor vim \
    && chsh -s zsh

SHELL ["zsh", "-c"]
# install oy-my-zsh
RUN sh -c "$(curl -fsSL https://ghproxy.com/https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" \
    && echo 'export EDITOR=vim' >> $HOME/.zshrc \
    # install rust toolchain
    && echo 'export RUSTUP_DIST_SERVER=https://mirror.sjtu.edu.cn/rust-static' >> $HOME/.zshrc \
    && echo 'export RUSTUP_UPDATE_ROOT=https://mirror.sjtu.edu.cn/rust-static/rustup' >> $HOME/.zshrc \
    && source $HOME/.zshrc \
    && curl -SL https://mirrors.tuna.tsinghua.edu.cn/rustup/rustup/archive/1.24.3/x86_64-unknown-linux-gnu/rustup-init >> rustup-init \
    && chmod a+x rustup-init && ./rustup-init --default-toolchain nightly -y && rm -rf rustup-init \
    && source $HOME/.cargo/env \
    && echo -e '[source]\n\n[source.mirror]\nregistry = "https://mirrors.sjtug.sjtu.edu.cn/git/crates.io-index/"\n\n[source.crates-io]\nreplace-with = "mirror"' \
    >> $HOME/.cargo/config.toml \
    && echo -e '[target.x86_64-unknown-linux-gnu]\nrustflags = ["-C", "linker=clang"]' >> $HOME/.cargo/config.toml \
    && cargo install starship && echo 'eval "$(starship init zsh)"' >> $HOME/.zshrc \
    && rm -rf $HOME/.cargo/registry \
    # install liburing
    && cd $HOME && git clone https://mirror.ghproxy.com/https://github.com/axboe/liburing.git && cd liburing && ./configure && make install && make clean && cd $HOME \
    # enable coredump
    && echo 'ulimit -c unlimited' >> $HOME/.zshrc \
    && mkdir /var/crash && echo 'echo /var/crash/core.%e >> /proc/sys/kernel/core_pattern' >> $HOME/.zshrc \
    && git clone https://mirror.ghproxy.com/https://github.com/ddxy18/xyco.git && cd xyco && git remote set-url origin https://github.com/ddxy18/xyco.git \
    && scripts/setup.sh \
    && apt-get remove -y curl pkg-config gcc && apt-get autoremove -y

CMD [ "zsh" ]
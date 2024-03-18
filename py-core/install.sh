#!/bin/bash -e

# Raspberry Pi で`py_core`を動かすための環境構築スクリプト
# $HOME/.bashrcに追記を行うため注意

# install asdf
if [ ! -d ~/.asdf ]; then
    git clone https://github.com/asdf-vm/asdf.git ~/.asdf --branch v0.14.0
    echo -e '\n. $HOME/.asdf/asdf.sh' >> ~/.bashrc
fi

. "$HOME/.asdf/asdf.sh"

# install python if command not exists
# this takes a long time
if ! command -v python >/dev/null; then
    # install python build dependencies
    sudo apt-get install build-essential gdb lcov pkg-config \
          libbz2-dev libffi-dev libgdbm-dev libgdbm-compat-dev liblzma-dev \
          libncurses5-dev libreadline6-dev libsqlite3-dev libssl-dev \
          lzma lzma-dev tk-dev uuid-dev zlib1g-dev

    asdf plugin add python
    asdf install python 3.12.0
    asdf global python 3.12.0
fi

# install nodejs
if ! command -v node >/dev/null; then
    asdf plugin add nodejs
    asdf install nodejs 21.0.0
    asdf global nodejs 21.0.0
fi

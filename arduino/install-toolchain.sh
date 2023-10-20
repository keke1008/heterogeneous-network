#!/bin/bash -eu

ORIGINAL_PACKAGE_JSON_PATH=~/.platformio/packages/toolchain-atmelavr/package.json

if [ ! -e "${ORIGINAL_PACKAGE_JSON_PATH}" ]; then
    echo "package.jsonが存在しません．"
    exit 1
fi

ROOT_DIR=$(cd $(dirname $0); pwd)
TOOLCHAIN_DIR="${ROOT_DIR}/toolchain"
AVR_GCC_DIR="${TOOLCHAIN_DIR}/avr-gcc"
DOWNLOAD_PATH="${TOOLCHAIN_DIR}/avr-gcc.tar.bz2"

echo "avr-gccのダウンロード中..."
mkdir -p ${TOOLCHAIN_DIR}
wget \
    -O ${DOWNLOAD_PATH} \
    https://github.com/ZakKemble/avr-gcc-build/releases/download/v12.1.0-1/avr-gcc-12.1.0-x64-linux.tar.bz2

echo "avr-gccの展開中..."
mkdir -p ${AVR_GCC_DIR}
tar -jxvf ${DOWNLOAD_PATH} -C ${AVR_GCC_DIR} --strip-components 1
rm ${DOWNLOAD_PATH}

echo "package.jsonのコピー中..."
cp ${ORIGINAL_PACKAGE_JSON_PATH} ${AVR_GCC_DIR}

echo "完了！"

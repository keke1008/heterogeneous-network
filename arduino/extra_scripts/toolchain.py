import os
import tarfile
import urllib.request
import shutil
from typing import Any
from pathlib import Path

env: Any = DefaultEnvironment()  # type: ignore
PROJECT_DIR = Path(env["PROJECT_DIR"])
TOOLCHAIN_EXTRACT_DIR = PROJECT_DIR / "toolchain"
TOOLCHAIN_EXTRACTED_ROOT_DIR = TOOLCHAIN_EXTRACT_DIR / "avr-gcc-12.1.0-x64-linux"
TOOLCHAIN_ROOT_DIR = TOOLCHAIN_EXTRACT_DIR / "avr-gcc"

DOWNLOAD_URL = f"https://github.com/ZakKemble/avr-gcc-build/releases/download/v12.1.0-1/avr-gcc-12.1.0-x64-linux.tar.bz2"


def is_toolchain_installed() -> bool:
    """
    ツールチェーンがインストールされているかどうかを返す．
    """
    return TOOLCHAIN_ROOT_DIR.exists()


def download_toolchain() -> str:
    """
    ツールチェーンをダウンロードし，ファイルのパスを返す．
    ダウンロードに失敗した場合は例外を投げる．
    """

    progress_percent = 0

    def report_hook(count, block_size, total_size):
        nonlocal progress_percent
        percent = int(count * block_size * 100 / total_size)
        if percent > progress_percent:
            progress_percent = percent
            print(f"ダウンロード中... {progress_percent}%")

    print("ツールチェーンをダウンロードしています．")

    try:
        path, _ = urllib.request.urlretrieve(DOWNLOAD_URL, None, report_hook)
        print("ツールチェーンのダウンロードが完了しました．")
        print(f"ダウンロードしたファイルのパス: {path}")
        return path
    except Exception as e:
        print(f"ツールチェーンのダウンロードに失敗しました． {e}")
        raise


def extract_toolchain(download_file_path: str) -> None:
    """
    ダウンロードしたツールチェーンを解凍する．
    解凍に失敗した場合は例外を投げる．
    """
    print("ツールチェーンを解凍しています．")

    try:
        with tarfile.open(download_file_path, "r:bz2") as tar:

            def progress():
                for member in tar.getmembers():
                    member.linkname
                    print(f"解凍中... {member.path}, {member.name}")
                    yield member

            tar.extractall(TOOLCHAIN_EXTRACT_DIR, members=progress())
            TOOLCHAIN_EXTRACTED_ROOT_DIR.rename(TOOLCHAIN_ROOT_DIR)

        print("ツールチェーンの解凍が完了しました．")
    except Exception as e:
        print(f"ツールチェーンの解凍に失敗しました． {e}")
        try:
            shutil.rmtree(TOOLCHAIN_EXTRACTED_ROOT_DIR)
            shutil.rmtree(TOOLCHAIN_ROOT_DIR)
        except Exception:
            pass
        raise


def install_package_json():
    package_json_content = """{
        "name": "toolchain-atmelavr",
        "version": "1.70300.1910",
        "description": "GCC Toolchain for Microchip AVR microcontrollers",
        "keywords": [
            "toolchain",
            "build tools",
            "compiler",
            "assembler",
            "linker",
            "preprocessor",
            "microchip",
            "avr"
        ],
        "homepage": "https://gcc.gnu.org/wiki/avr-gcc",
        "license": "GPL-2.0-or-later",
        "system": [
            "linux_x86_64"
        ]
    }"""
    with open(TOOLCHAIN_ROOT_DIR / "package.json", "w") as f:
        f.write(package_json_content)


def install_toolchain(*args, **kwargs):
    if is_toolchain_installed():
        print("ツールチェーンは既にインストールされています．")
        return

    file_path = download_toolchain()

    try:
        extract_toolchain(file_path)
        install_package_json()
        print("ツールチェーンのインストールが完了しました．")
    except Exception:
        raise
    finally:
        try:
            os.remove(file_path)
        except Exception:
            pass


env.AddCustomTarget(
    "install_toolchain",
    None,
    install_toolchain,
    title="install toolchain",
    description="install AVR C++20 toolchain をインストールします．",
)


def uninstall_toolchain(*args, **kwargs):
    if not is_toolchain_installed():
        print("ツールチェーンはインストールされていません．")
        return

    shutil.rmtree(TOOLCHAIN_ROOT_DIR)
    print("ツールチェーンのアンインストールが完了しました．")


env.AddCustomTarget(
    "uninstall_toolchain",
    None,
    uninstall_toolchain,
    title="uninstall toolchain",
    description="AVR C++20 toolchain をアンインストールします．",
)

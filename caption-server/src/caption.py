import os
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Final
from io import BytesIO

_SCRIPT_DIR_PATH: Final[Path] = Path(__file__).parent

_CURRENT_DIR_PATH: Final[Path] = _SCRIPT_DIR_PATH.parent / "scripts" / "synthesistools"
os.chdir(_CURRENT_DIR_PATH)

_SHOW_CAPTION_SCRIPT_NAME: Final[Path] = _CURRENT_DIR_PATH / "csimage.sh"
_CLEAR_CAPTION_SCRIPT_NAME: Final[Path] = _CURRENT_DIR_PATH / "synthesis-clear.sh"


@dataclass
class Caption:
    image: BytesIO
    x: int
    y: int


_IMAGE_FILE_PATH: Final[Path] = _SCRIPT_DIR_PATH.parent / "image" / "image.png"


def show_caption(caption: Caption) -> None:
    with open(_IMAGE_FILE_PATH, "wb") as f:
        f.write(caption.image.getbuffer())

    commands = [
        str(_SHOW_CAPTION_SCRIPT_NAME),
        str(_IMAGE_FILE_PATH),
        str(caption.x),
        str(caption.y),
    ]
    proc = subprocess.run(commands)
    if proc.returncode != 0:
        raise Exception(f"Error csimage.sh exited with code {proc.returncode}")


def clear_caption() -> None:
    commands = [str(_CLEAR_CAPTION_SCRIPT_NAME)]
    proc = subprocess.run(commands)
    if proc.returncode != 0:
        raise Exception(f"Error synthesis-clear.sh exited with code {proc.returncode}")

from datetime import timedelta
from typing import Final


SEND_HELLO_INTERVAL: Final[timedelta] = timedelta(seconds=10)
NEIGHBOR_EXPIRATION_TIMEOUT: Final[timedelta] = SEND_HELLO_INTERVAL * 4

#/usr/bin/env bash -e

CURRENT_DIR="$(dirname "${0}")"

${CURRENT_DIR}/detach.sh "${@}"

sleep 0.5

${CURRENT_DIR}/attach.sh "${@}"

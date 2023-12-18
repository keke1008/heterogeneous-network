#/usr/bin/env bash -e

if [ "${#}" -eq 0 ]; then
    echo "Usage: ${0} <COM port number> [<COM port number> ...]"
    exit 1
fi

for name in "${@}"; do
    usbipd.exe attach --wsl --busid "$(usbipd.exe list | grep "COM${name}" | cut -d ' ' -f 1)"
done

wait

sleep 1

sudo chmod o+rw /dev/ttyACM*

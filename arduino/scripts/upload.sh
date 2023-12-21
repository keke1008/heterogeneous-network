#/usr/bin/env bash -e

pio run -e debug

for port in $( find /dev/ -name 'ttyACM*' ); do
    pio run -e debug -t upload -t nobuild --upload-port ${port} &
done

wait

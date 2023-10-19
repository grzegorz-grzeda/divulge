#!/bin/bash
set -e
PORT=5000
COUNT=100000

for i in $(seq 1 1 $COUNT)
do
    curl http://localhost:$PORT/ -s >/dev/null &
done
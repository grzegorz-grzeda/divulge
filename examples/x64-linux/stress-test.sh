#!/bin/bash
set -e
PORT=5000
COUNT=1000

for i in $(seq 1 1 $COUNT)
do
    $(time curl http://localhost:$PORT/ -s >/dev/null )&
done
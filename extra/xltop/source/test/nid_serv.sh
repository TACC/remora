#!/bin/bash

addr=localhost:9901
prog=$(basename $0)

if [ $# -ne 2 ]; then
    echo "Usage: ${prog} NID SERV" >&2
    exit 1
fi

nid="$1"
serv="$2"

v_arg=""
if [ -n "$V" ]; then
    v_arg="-v"
fi

while true; do
    echo $nid 1048576 1024 1 | \
        curl $v_arg --data-binary @- -XPUT http://${addr}/serv/${serv}
    sleep 1
done

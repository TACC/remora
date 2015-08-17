#!/bin/bash

addr=localhost:9901
nid_file=test/client-nids
prog=$(basename $0)

if [ $# -ne 1 ]; then
    echo "Usage: ${prog} N" >&2
    exit 1
fi

serv="mds$1.ranger.tacc.utexas.edu"

v_arg=""
if [ -n "$V" ]; then
    v_arg="-v"
fi

function fake_loadavg {
    echo $((RANDOM % 100)).$((RANDOM % 100)) \
        $((RANDOM % 100)).$((RANDOM % 100)) \
        $((RANDOM % 100)).$((RANDOM % 100))
}

function status {
    echo $(date +%s) \
         $(awk '{ print $1 }' /proc/uptime) \
         $(fake_loadavg) \
         0 0 0 0 0 0 \
         $(awk -F'[ |/]' '{ print $5 }' /proc/loadavg) \
         1 0 4037
}

while true; do
    (
        shuf $nid_file | sed 1024q | while read nid; do
            echo $nid 0 0 $((RANDOM / 1024))
        done
    ) | curl $v_arg --data-binary @- -XPUT http://${addr}/serv/${serv}
    status | curl $v_arg --data-binary @- -XPUT http://${addr}/serv/${serv}/_status &>/dev/null
    sleep 30
done

#!/bin/bash

# xid=$(printf "%llx" $RANDOM)
user="blarf"
domain="ranger.tacc.utexas.edu"
now=$(date +%s)
auth=11

LINES=$(tput lines)

function xid {
    printf "%llx" $(((RANDOM << 15) + RANDOM)) #
}

(
    echo %user_connect $(xid) ${user} ${domain} ${now} ${auth}
    echo %dump $(xid) $@
) | nc -q 1 localhost 9901 | grep -v ^%

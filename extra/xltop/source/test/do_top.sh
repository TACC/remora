#!/bin/bash

addr=localhost:9901
prog=$(basename $0)

if [ $# -ne 4 ]; then
    echo "Usage: ${prog} X0 X1 D0 D1" >&2
    exit 1
fi

v_arg=""
if [ -n "$V" ]; then
    v_arg="-v"
fi

x0=$1
x1=$2
d0=$3
d1=$4

if [ -z "$limit" ]; then
    limit=$(tput lines)
    limit=$((limit - 2))
fi

sort_q=""
if [ -n "$sort" ]; then
    sort_q='&sort='
fi

query="x0=${x0}&x1=${x1}&d0=${d0}&d1=${d1}&limit=${limit}${sort_q}${sort}"

curl $v_arg "http://${addr}/top?${query}"

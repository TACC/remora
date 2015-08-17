#!/bin/bash

addr=localhost:9901
clus="ranger"
prog=$(basename $0)

v_arg=""
if [ -n "$V" ]; then
    v_arg="-v"
fi

n=${n:-0}

./qhost < test/qhost-j.${n} | curl $v_arg --data-binary @- -XPUT http://${addr}/clus/${clus}

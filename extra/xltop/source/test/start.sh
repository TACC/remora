#!/bin/bash

killall -q do_mds.sh do_oss.sh

./test/do_clus.sh

./test/do_mds.sh 1 &
./test/do_mds.sh 3 &
./test/do_mds.sh 5 &

for ((i = 1; i <= 20; i++)); do
    (./test/do_oss.sh $i &)
    sleep 1
done

for ((i = 23; i <= 72; i++)); do
    (./test/do_oss.sh $i &)
    sleep 1
done

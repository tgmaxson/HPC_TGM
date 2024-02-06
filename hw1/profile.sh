#! /bin/bash

for size in 1000 5000 10000
do 
  for gens in 1000 5000
  do 
    ./bin/hw1-gcc-O3 $size $gens 0
    ./bin/hw1-icc-O3 $size $gens 0
  done
done

./bin/hw1-gcc-O0 1000 1000 0
./bin/hw1-gcc-O0 1000 5000 0
./bin/hw1-gcc-O0 5000 1000 0
./bin/hw1-gcc-O0 5000 5000 0




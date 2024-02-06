#! /bin/bash

mkdir bin

gcc -o bin/hw1-gcc-O0 hw1.c -lm -Wall -O0
gcc -o bin/hw1-gcc-O3 hw1.c -lm -Wall -O3
icc -o bin/hw1-icc-O3 hw1.c -lm -Wall -O3
#! /bin/bash

mkdir bin

gcc -lm -Wall -O0 -o bin/hw1-gcc-O0 hw1.c
gcc -lm -Wall -O3 -o bin/hw1-gcc-O3 hw1.c
icc -lm -Wall -O3 -o bin/hw1-icc-O3 hw1.c
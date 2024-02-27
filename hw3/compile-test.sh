#! /bin/bash

mkdir bin

gcc -o bin/hw3 hw3.c -lm -Wall -O3 -fopenmp

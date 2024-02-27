#! /bin/bash

mkdir bin

gcc -o bin/hw3-matrix hw3-matrix.c -fopenmp -lm -Wall -O3 

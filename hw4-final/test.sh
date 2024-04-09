#! /bin/bash
#SBATCH -J ProfileProgram
#SBATCH -N 1
#SBATCH -p main
#SBATCH --time=4:00:00
#SBATCH --ntasks-per-node=32

source ~/.bashrc
mamba activate work

rm bin/hw4
mpicc -g -Wunused-variable -o bin/hw4 hw4.c -lm -Wall -O3 

mpirun -np 4 ./bin/hw4 2000 2000 test.txt 0

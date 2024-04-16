#! /bin/bash
#SBATCH -J ProfileProgram
#SBATCH -N 1
#SBATCH -p main
#SBATCH --time=4:00:00
#SBATCH --ntasks-per-node=32

source ~/.bashrc
mamba activate work

rm bin/simple-hw5
nvcc -o bin/simple-hw5 simple-hw5.cu -lm -O3 

for sizes in 100 500 1000 2000 4000 8000 12000 15000
do
  ./bin/simple-hw5 $sizes 100 test.txt 0 cpu
  for threads in 8 16 32 64 128 256 512 1024 2048
  do 
    ./bin/simple-hw5 $sizes 100 test.txt 0 gpu $threads
  done
done

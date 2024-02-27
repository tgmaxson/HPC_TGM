#! /bin/bash
#SBATCH -J ProfileProgram
#SBATCH -N 1
#SBATCH -p main
#SBATCH --time=4:00:00
#SBATCH --ntasks-per-node=32

source ~/.bashrc
mamba activate work

module load intel/compiler intel/mkl/ intel/mpi

for size in 1000 5000 10000
do 
  for gens in 1000 5000
  do 
    ./bin/hw1-gcc-O3 $size $gens 0 &
    sleep 1
    ./bin/hw1-icc-O3 $size $gens 0 &
    sleep 1
    ./bin/hw1-gcc-O0 $size $gens 0 &
    sleep 1
  done
done

wait
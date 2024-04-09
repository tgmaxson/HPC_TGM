#! /bin/bash
#SBATCH -J ProfileProgram
#SBATCH -N 1
#SBATCH -p main
#SBATCH --time=4:00:00
#SBATCH --ntasks-per-node=28

source ~/.bashrc
mamba activate work

rm bin/hw4
mpicc -o bin/hw4 hw4.c -lm -Wall -O1

rm performance.txt

for size in 1500 2000 2500 3000 3500 4000 5000 7500
do 
  for tasks in 1 2 4 8 12 16 20 24 28
  do
    for gens in 200
    do 
      for replicates in {1..3}
      do
        mpirun -np $tasks ./bin/hw4 $size $gens output-$size-$gens.txt 0 >> performance.txt
      done
    done
  done
done

#! /bin/bash
#SBATCH -J ProfileProgram
#SBATCH -N 3
#SBATCH -p main
#SBATCH --time=4:00:00
#SBATCH --ntasks-per-node=2
#SBATCH --gres=gpu:rtx-4090:2


# Cluster Specific
source ~/.bashrc
mamba activate work

# Setup
module load nvhpc
rm -r bin build
mkdir bin build

# Make program
exe="hw6"
mpicc -c hw6.c -o build/hw6.o -lm -O3          # Main Program with MPI
nvcc  -c game-kernel.cu -o build/game-kernel.o # GPU Kernel + C binding
mpicc build/hw6.o build/game-kernel.o -o bin/$exe -lm -O3 -lcudart -L/opt/nvidia/hpc_sdk/Linux_x86_64/23.3/cuda/12.0/lib64/

echo "Built Hw6!  Verify it's presence!"
echo "------------"
ls bin
echo "------------"
echo "Start Benchmarking!"

for sizes in 100 500 1000 2000 4000 8000 12000 15000 30000
do
    # No openib, exclude it on our cluster
    mpirun --mca btl '^openib' -np $SLURM_NTASKS ./bin/${exe} ${sizes} 1000 ${sizes}.txt 0 64
done

#include <ctype.h>
#include <cuda_runtime.h>

// CUDA Kernel
__global__ void gameKernel(int *current_board, int *next_board, int width, int height) {
    int i = blockIdx.y * blockDim.y + threadIdx.y;
    int j = blockIdx.x * blockDim.x + threadIdx.x;

    if (i < height && j < width) {
        int idx = (i + 1) * (width + 2) + (j + 1);  // Adjusted for 1-padded border

        // Calculate the number of alive neighbors
        int neighbors = 0;
        neighbors += current_board[idx - (width + 2) - 1];
        neighbors += current_board[idx - (width + 2)];
        neighbors += current_board[idx - (width + 2) + 1];
        neighbors += current_board[idx - 1];
        neighbors += current_board[idx + 1];
        neighbors += current_board[idx + (width + 2) - 1];
        neighbors += current_board[idx + (width + 2)];
        neighbors += current_board[idx + (width + 2) + 1];

        // Apply the Game of Life rules
        if (current_board[idx] == 1) {
            next_board[idx] = (neighbors == 2 || neighbors == 3) ? 1 : 0;
        } else {
            next_board[idx] = (neighbors == 3) ? 1 : 0;
        }
    }
}


extern "C" void callGameKernel(int *gpu_current_board, int *gpu_next_board, int width, int height, int THREADS)
{
    dim3 threadsPerBlock(THREADS, THREADS);
    dim3 blocksPerGrid((width  + threadsPerBlock.x - 1) / threadsPerBlock.x, 
                       (height + threadsPerBlock.y - 1) / threadsPerBlock.y);
    gameKernel<<<blocksPerGrid, threadsPerBlock>>>(gpu_current_board, gpu_next_board, width, height);
}


extern "C" void free_GPU(int *gpu_data)
{
    cudaFree(gpu_data); // Error handling here would be good
}


extern "C" int* allocate_GPU(int width, int height) {
    int *gpu_data;
    cudaMalloc(&gpu_data, (width + 2) * (height + 2) * sizeof(int)); // Error handling here would be good
    return gpu_data;
}


extern "C" void data_to_GPU(int *host_data, int* gpu_data, int size, int offset) {
    int* host_data_offset = host_data + offset;
    cudaMemcpy(gpu_data, host_data_offset, size * sizeof(int), cudaMemcpyHostToDevice); // Error handling here would be good
}


extern "C" void data_from_GPU(int *host_data, int* gpu_data, int size, int offset) {
    int* gpu_data_offset = gpu_data + offset;
    cudaMemcpy(host_data, gpu_data_offset, size * sizeof(int), cudaMemcpyDeviceToHost); // Error handling here would be good
}
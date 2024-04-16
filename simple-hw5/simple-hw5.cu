#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <ctype.h>
#include <cuda_runtime.h>

// Make it easy to switch types for the game board
#define BOARD_TYPE float

// This isn't strictly required I think, header file is somehow useful here?
void initializeBoard(BOARD_TYPE *board, int size_x, int size_y, int seed);
void printBoard(BOARD_TYPE *board, int size_x, int size_y, FILE *file);
void swapBoard(BOARD_TYPE **board1, BOARD_TYPE **board2);
BOARD_TYPE *allocate_array(int Q, int P);
void free_array(BOARD_TYPE *a);
double get_time();
int index_2d(int row, int col, int num_cols);
char* to_lowercase(const char *str);
BOARD_TYPE* data_to_GPU(BOARD_TYPE *host_data, int size);

// CUDA Kernel
__global__ void gameKernel(BOARD_TYPE *current_board, BOARD_TYPE *next_board, int N) {
    int i = blockIdx.y * blockDim.y + threadIdx.y + 1;  // Adjust index for 1-based
    int j = blockIdx.x * blockDim.x + threadIdx.x + 1;  // Adjust index for 1-based

    if (i <= N && j <= N) {  // Ensure we don't go out of bounds
        int idx = i * (N + 2) + j;  // +2 for the border padding
        int neighbors = 0;

        // Calculate the number of alive neighbors
        // This assumes current_board is padded by 1 on all sides
        neighbors += current_board[idx - N - 3];  // top-left
        neighbors += current_board[idx - N - 2];  // top
        neighbors += current_board[idx - N - 1];  // top-right
        neighbors += current_board[idx - 1];      // left
        neighbors += current_board[idx + 1];      // right
        neighbors += current_board[idx + N + 1];  // bottom-left
        neighbors += current_board[idx + N + 2];  // bottom
        neighbors += current_board[idx + N + 3];  // bottom-right

        // Apply the Game of Life rules
        if (current_board[idx] == 1) {
            next_board[idx] = (neighbors == 2 || neighbors == 3) ? 1 : 0;
        } else {
            next_board[idx] = (neighbors == 3) ? 1 : 0;
        }
    }
}

int main(int argc, char *argv[]) {
    const int N = atoi(argv[1]); // Board size (no PBC)
    const int MAX_GENERATIONS = atoi(argv[2]); // Max generations
    const char *OUTPUT_FILENAME = argv[3];
    const int SEED = atoi(argv[4]); // Random Seed
    const char *DEVICE = to_lowercase(argv[5]);
    int THREADS = 1;

    // Allocate and scatter big array
    BOARD_TYPE *current_board = allocate_array(N, N);
    BOARD_TYPE *next_board = allocate_array(N, N);
    initializeBoard(current_board, N, N, SEED);

    // Game loop
    const double start_time = get_time();

    //printBoard(current_board, N, N, stdout);

    if (strcmp(DEVICE, "cpu") == 0) {
        // CPU version of codee
        for (int gen = 0; gen < MAX_GENERATIONS; ++gen) {
            for (int i = 1; i <= N; i++) { // Bounds to only look at board without PBC to avoid issues
                for (int j = 1; j <= N; j++) {
                    const int aliveNeighbors  = (current_board[index_2d(i - 1, j - 1, N)] + current_board[index_2d(i    , j - 1, N)] + current_board[index_2d(i + 1, j - 1, N)] +
                                                 current_board[index_2d(i - 1, j    , N)] +                                          + current_board[index_2d(i + 1, j    , N)] +
                                                 current_board[index_2d(i - 1, j + 1, N)] + current_board[index_2d(i    , j + 1, N)] + current_board[index_2d(i + 1, j + 1, N)]);

                    // Game of life
                    if (current_board[index_2d(i, j, N)] == 1) { // Alive cell
                        if (aliveNeighbors < 2 || aliveNeighbors > 3) {
                            next_board[index_2d(i, j, N)] = 0; // Dies
                        } else {
                            next_board[index_2d(i, j, N)] = 1; // Lives
                        }
                    } else { // Dead cell
                        if (aliveNeighbors == 3) {
                            next_board[index_2d(i, j, N)] = 1; // Becomes alive
                        } else {
                            next_board[index_2d(i, j, N)] = 0; // Stays dead
                        }
                    }
                }
            }       
            swapBoard(&next_board, &current_board); // Shuffle it in
        }
    } else if (strcmp(DEVICE, "gpu") == 0) {
        // Code for GPU

        BOARD_TYPE *gpu_current_board = data_to_GPU(current_board, (N + 2) * (N + 2));
        BOARD_TYPE *gpu_next_board = data_to_GPU(next_board, (N + 2) * (N + 2));

        // Read number of threads
        THREADS = atoi(argv[6]); // Max generations

        dim3 threadsPerBlock(THREADS, THREADS);
        dim3 blocksPerGrid((N + threadsPerBlock.x - 1) / threadsPerBlock.x, 
                           (N + threadsPerBlock.y - 1) / threadsPerBlock.y);

        for (int gen = 0; gen < MAX_GENERATIONS; ++gen) {
            gameKernel<<<blocksPerGrid, threadsPerBlock>>>(gpu_current_board, gpu_next_board, N);
            cudaDeviceSynchronize();

            swapBoard(&gpu_next_board, &gpu_current_board); // Shuffle it in
        }
        // Copy back
        cudaMemcpy(current_board, gpu_current_board, (N + 2) * (N + 2) * sizeof(int), cudaMemcpyDeviceToHost);

        // Free up the GPU memory
        cudaFree(gpu_current_board);
        cudaFree(gpu_next_board);
    } else {
        printf("Running on an unspecified device.\n");
        return 1;
    }

    //printBoard(current_board, N, N, stdout);

    const double end_time = get_time();
    const double time_spent = end_time - start_time;

    FILE *outputFile = fopen(OUTPUT_FILENAME, "w");
    if (outputFile == NULL) {
        perror("Error opening output file");
        return 1; // Error code
    }
    else {
        printBoard(current_board, N, N, outputFile);
    }
    fclose(outputFile);

    printf("EXE: %s, SIZE: %d, GENS: %d, SEC: %f, DEVICE: %s, THREADS: %d\n", argv[0], N, MAX_GENERATIONS, time_spent, DEVICE, THREADS);


    return 0;
}


BOARD_TYPE* data_to_GPU(BOARD_TYPE *host_data, int size) {
    BOARD_TYPE *gpu_data;
    cudaMalloc(&gpu_data, size * sizeof(BOARD_TYPE));
    cudaMemcpy(gpu_data, host_data, size * sizeof(BOARD_TYPE), cudaMemcpyHostToDevice);
    return gpu_data;
}


char* to_lowercase(const char *str) {
    // Function not really needed strictly, but surprised how complicated this is.
    if (str == NULL) return NULL;

    char *lowercase_str = (char *)malloc(strlen(str) + 1);
    if (lowercase_str == NULL) return NULL;

    char *p = lowercase_str;
    while (*str) {
        *p++ = tolower((unsigned char) *str);
        str++;
    }
    *p = '\0';

    return lowercase_str;
}


int index_2d(int row, int col, int num_cols) {
    return row * num_cols + col;
}


double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    long long milliseconds = (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return (double)milliseconds / 1000;
}


void free_array(BOARD_TYPE *a) {
  free(a);
}


BOARD_TYPE* allocate_array(int Q, int P) {
    Q += 2; // Padding
    P += 2; 
    // Allocate a single contiguous array
    BOARD_TYPE *a = (BOARD_TYPE *)calloc(P * Q, sizeof(BOARD_TYPE));
    if (a == NULL) {
        printf("Error allocating memory\n");
        return NULL;
    }
    return a;
}


void swapBoard(BOARD_TYPE **board1, BOARD_TYPE **board2) {
    BOARD_TYPE *tempBoard = *board1;
    *board1 = *board2;
    *board2 = tempBoard;
}


void initializeBoard(BOARD_TYPE *board, int size_x, int size_y, int seed) {
    size_x += 2; // Padding
    size_y += 2;

    srand(seed);

    // Only initialize the inner area, skip the padding
    for (int i = 1; i < size_y - 1; i++) {
        for (int j = 1; j < size_x - 1; j++) {
            board[i * size_x + j] = rand() % 2; // Randomly 0 or 1
        }
    }
}


void printBoard(BOARD_TYPE *board, int size_x, int size_y, FILE *file) {
    size_x += 2; // Padding
    size_y += 2;

    // Print a row of dashes
    for (int j = 0; j < size_x * 2 + 1; j++) {
        fprintf(file, "-");
    }
    fprintf(file, "\n");

    // Make cells wider to make board more square
    for (int i = 0; i < size_y; i++) { 
        for (int j = 0; j < size_x; j++) { 
            int index = i * size_x + j;
            fprintf(file, "%c ", board[index] ? 'X' : ' '); // 'X' for alive, 'O' for dead
        }
        fprintf(file, "\n");
    }

    // Print a row of dashes
    for (int j = 0; j < size_x * 2 + 1; j++) {
        fprintf(file, "-");
    }
    fprintf(file, "\n");
}

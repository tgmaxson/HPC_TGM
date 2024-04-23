#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <mpi.h>
#include <cuda_runtime.h>

int **initializeBoard(int **board, int size_x, int size_y, int seed);
void printBoard(int **board, int size_x, int size_y);
void swapBoard(int **board1, int **board2);
void writeBoardToFile(int **board, int size_x, int size_y, FILE *file);
int **allocate_array(int Q, int P);
void free_array(int **a);
double get_time();
int min(int x, int y);
int *allocate_GPU(int width, int height);
void data_to_GPU(int *host_data, int* gpu_data, int size, int offset);
void data_from_GPU(int *host_data, int* gpu_data, int size, int offset);
void free_GPU(int *gpu_data);
void callGameKernel(int *gpu_current_board, int *gpu_next_board, int width, int height, int THREADS);


int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int WORLD_SIZE, WORLD_RANK;
    MPI_Comm_size(MPI_COMM_WORLD, &WORLD_SIZE);
    MPI_Comm_rank(MPI_COMM_WORLD, &WORLD_RANK);

    const int N_X = atoi(argv[1]); // Board size (no PBC)
    const int MAX_GENERATIONS = atoi(argv[2]); // Max generations
    const char *OUTPUT_FILENAME = argv[3];
    const int SEED = atoi(argv[4]); // Random Seed

    // Define ranks to communicate with
    const int prev_rank = (WORLD_RANK - 1 < 0) ? MPI_PROC_NULL : WORLD_RANK - 1;
    const int next_rank = (WORLD_RANK + 1 >= WORLD_SIZE) ? MPI_PROC_NULL : WORLD_RANK + 1;

    // Calculate how many rows this process should handle
    const int rows_per_worker = N_X / WORLD_SIZE;
    const int remaining_rows = N_X % WORLD_SIZE;

    int N_before = 0;
    // Calculate N for this worker and row number
    for (int i=0; i < WORLD_RANK; i++) {
        N_before += rows_per_worker + (i < remaining_rows);
    }
    const int N_BEFORE = N_before;
    const int N_Y = rows_per_worker + (WORLD_RANK < remaining_rows);

    // Allocate memory for the part of the board local to this worker
    int **current_board = allocate_array(N_X, N_Y);
    int **next_board = allocate_array(N_X, N_Y);

    current_board = initializeBoard(current_board, N_X, N_Y, SEED + N_BEFORE);

    int NUM_GPUS;
    cudaGetDeviceCount(&NUM_GPUS);
    cudaSetDevice(WORLD_RANK % NUM_GPUS);

    int *current_board_GPU = allocate_GPU(N_X, N_Y);
    int *next_board_GPU = allocate_GPU(N_X, N_Y);

    const int board_size = (N_X + 2) * (N_Y + 2);
    const int row_size = (N_X + 2);
    const int row_last_offset = (N_Y) * row_size;

    data_to_GPU(current_board[0], current_board_GPU, board_size, 0);

    
    // Game loop
    double start_time = get_time();
    for (int gen = 0; gen < MAX_GENERATIONS; ++gen) {
        if (prev_rank != MPI_PROC_NULL) {
            MPI_Sendrecv(current_board[1], N_X, MPI_INT, prev_rank, 0,
                         current_board[N_Y + 1], N_X, MPI_INT, prev_rank, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        if (next_rank != MPI_PROC_NULL) {
            MPI_Sendrecv(current_board[N_Y], N_X, MPI_INT, next_rank, 0,
                         current_board[0], N_X, MPI_INT, next_rank, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        data_to_GPU(current_board[0], current_board_GPU, row_size, 0);
        data_to_GPU(current_board[N_Y + 1], current_board_GPU, row_size, 0);

        callGameKernel(current_board_GPU, next_board_GPU, N_X, N_Y, 64);

        data_from_GPU(current_board[0], current_board_GPU, row_size, 0);
        data_from_GPU(current_board[N_Y + 1], current_board_GPU, row_size, 0);

        swapBoard(&current_board_GPU, &next_board_GPU);
        swapBoard(next_board, current_board); // Shuffle it in
    }
    
    double end_time = get_time();
    const double time_spent = end_time - start_time;

    double max_time;
    MPI_Reduce(&time_spent, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    
    // Buffer to hold the gathered data on each process
    int *N_before_all = malloc(WORLD_SIZE * sizeof(int));
    int *N_Y_all = malloc(WORLD_SIZE * sizeof(int));

    // Perform the all-gather operation
    MPI_Allgather(&N_BEFORE, 1, MPI_INT, N_before_all, 1, MPI_INT, MPI_COMM_WORLD);
    MPI_Allgather(&N_Y, 1, MPI_INT, N_Y_all, 1, MPI_INT, MPI_COMM_WORLD);

    int **full_board = allocate_array(N_X + 2, N_X + 2);
    full_board = initializeBoard(full_board, N_X, N_X, SEED); // This is inefficent but PBC filling function should be split out

    for (int i = 0; i < N_Y; i++) { // Copy current to correct part of full
        full_board[N_BEFORE + i] = current_board[i];
    }
    
    for (int i = 0; i < WORLD_SIZE; i++) {
        //printf("start: %d\n", i);
        const int lower_bound = N_before_all[i];
        const int upper_bound = N_before_all[i] + N_Y_all[i];
        for (int j = lower_bound; j < upper_bound; j++) {
            MPI_Bcast(full_board[j], N_X + 2, MPI_INT, i, MPI_COMM_WORLD);
        }
        //printf("end: %d\n", i);
    }

    if (WORLD_RANK == 0) { // Final stuff only done on master rank
        FILE *outputFile = fopen(OUTPUT_FILENAME, "w");
        if (outputFile == NULL) {
            perror("Error opening output file");
            return 1; // Error code
        }
        else {
            writeBoardToFile(full_board, N_X, N_X, outputFile);
        }
        fclose(outputFile);

        double end_time = get_time();
        const double time_spent = end_time - start_time;

        printf("EXE: %s, RANKS: %d, SIZE: %d, GENS: %d, SEC: %f \n", argv[0], WORLD_SIZE, N_X, MAX_GENERATIONS, max_time);
    }
    
    MPI_Finalize();

    return 0;
}


int min(int x, int y) {
    return (x < y) ? x : y;
}


double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    long long milliseconds = (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return (double)milliseconds / 1000;
}


void free_array(int **a) {
  free(&a[0][0]);
  free(a);
}


int **allocate_array(int Q, int P) {
  int i;
  int *p, **a;

  Q += 2;
  P += 2;
  
  p = (int *)malloc(P*Q*sizeof(int));
  a = (int **)malloc(P*sizeof(int*));

  if (p == NULL || a == NULL) 
    printf("Error allocating memory\n");

  for (i = 0; i < P; i++)
    a[i] = &p[i*Q];
  
  return a;
}


void swapBoard(int **board1, int **board2) {
    int *tempBoard = *board1;
    *board1 = *board2;
    *board2 = tempBoard;
}


int **initializeBoard(int **board, int size_x, int size_y, int seed) {
    for (int j = 0; j < size_x + 2; j++) {
        board[0][j] = 0; // Top boundary
        board[size_y + 1][j] = 0; // Bottom boundary
    }

    for (int i = 1; i < size_y + 1; i++) {
        board[i][0] = 0; // Left boundary
        board[i][size_x + 1] = 0; // Right boundary
    }

    // Fill in the inner cells
    for (int i = 1; i < size_y + 1; i++) {
        srand(seed + i);
        for (int j = 1; j < size_x + 1; j++) {
            board[i][j] = rand() % 2; // Modulus to get 0 or 1
        }
    }

    return board;
}


void printBoard(int **board, int size_x, int size_y) {
    // Print a row of dashes
    for (int j = 0; j <= ((size_x + 1) * 2); j++) {
        printf("-");
    }
    printf("\n");

    // Make cells wider to make board more square
    for (int i = 0; i <= size_y + 1; i++) { // Skip PBC edge
        for (int j = 0; j <= size_x + 1; j++) {
            printf("%c ", board[i][j] ? 'X' : 'O'); // 'X' for alive, ' ' for dead
        }
        printf("\n");
    }

    // Print a row of dashes
    for (int j = 0; j <= ((size_x + 1) * 2); j++) {
        printf("-");
    }
    printf("\n");

}

void writeBoardToFile(int **board, int size_x, int size_y, FILE *file) {
    // Print a row of dashes
    for (int j = 0; j <= ((size_x + 1) * 2); j++) {
        fprintf(file, "-");
    }
    fprintf(file, "\n");

    // Make cells wider to make board more square
    for (int i = 0; i <= size_y + 1; i++) { // Skip PBC edge
        for (int j = 0; j <= size_x + 1; j++) {
            fprintf(file, "%c ", board[i][j] ? 'X' : 'O'); // 'X' for alive, ' ' for dead
        }
        fprintf(file, "\n");
    }

    // Print a row of dashes
    for (int j = 0; j <= ((size_x + 1) * 2); j++) {
        fprintf(file, "-");
    }
    fprintf(file, "\n");
}
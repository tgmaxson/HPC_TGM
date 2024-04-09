#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <mpi.h>

// This isn't strictly required I think, header file is somehow useful here?
int **initializeBoard(int **board, int size_x, int size_y, int seed);
void printBoard(int **board, int size_x, int size_y, FILE *file);
void swapBoard(int ***board1, int ***board2);
int **allocate_array(int Q, int P);
void free_array(int **a);
double get_time();
int min(int x, int y);


int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int WORLD_SIZE, WORLD_RANK;
    MPI_Comm_size(MPI_COMM_WORLD, &WORLD_SIZE);
    MPI_Comm_rank(MPI_COMM_WORLD, &WORLD_RANK);

    const int N = atoi(argv[1]); // Board size (no PBC)
    const int MAX_GENERATIONS = atoi(argv[2]); // Max generations
    const int PRINT_ALL = 0; // Old logic, we don't want to output the whole time
    const char *OUTPUT_FILENAME = argv[3];
    const int THREADS = 1; // Threads to use
    const int SEED = atoi(argv[4]); // Random Seed

    // Define ranks to communicate with
    const int prev_rank = (WORLD_RANK - 1 < 0) ? MPI_PROC_NULL : WORLD_RANK - 1;
    const int next_rank = (WORLD_RANK + 1 >= WORLD_SIZE) ? MPI_PROC_NULL : WORLD_RANK + 1;

    // Calculate how many rows this process should handle
    const int rows_per_worker = N / WORLD_SIZE;
    const int remaining_rows = N % WORLD_SIZE;

    int *sizes = (int *)calloc(WORLD_SIZE, sizeof(int*));
    int *displacements = (int *)calloc(WORLD_SIZE, sizeof(int*));

    int *sizes_flat = (int *)calloc(WORLD_SIZE, sizeof(int*));
    int *displacements_flat = (int *)calloc(WORLD_SIZE, sizeof(int*));

    // Calculate displacements
    for (int i=0; i < WORLD_SIZE; i++) {
        sizes[i] = rows_per_worker + (i < remaining_rows);
        if (i != 0) { displacements[i] += displacements[i - 1] + sizes[i - 1]; }
    }

    // Calculate flat
    for (int i = 0; i < WORLD_SIZE; i++) {
        sizes_flat[i] = sizes[i] * (N + 2);
        displacements_flat[i] = (displacements[i] * (N + 2));
    }

    const int N_X = N;
    const int N_Y = sizes[WORLD_RANK];

    // Allocate and scatter big array
    int **full_board = allocate_array(N + 2, N + 2);
    full_board = initializeBoard(full_board, N, N, SEED);

    // Allocate memory for the part of the board local to this worker
    int **current_board = allocate_array(N_X + 2, N_Y + 2);
    int **next_board = allocate_array(N_X + 2, N_Y + 2);

    //if (WORLD_RANK == 0) { printBoard(full_board, N, N, stdout); }

    MPI_Scatterv(&full_board[1][0], sizes_flat, displacements_flat, MPI_INT, &current_board[1][0], sizes_flat[WORLD_RANK], MPI_INT, 0, MPI_COMM_WORLD);

    // Game loop
    const double start_time = get_time();

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

        for (int i = 1; i <= N_Y; i++) { // Bounds to only look at board without PBC to avoid issues
            for (int j = 1; j <= N_X; j++) {
                const int aliveNeighbors  = current_board[i - 1][j - 1] + current_board[i    ][j - 1] + current_board[i + 1][j - 1] +
                                            current_board[i - 1][j    ] +                             + current_board[i + 1][j    ] +
                                            current_board[i - 1][j + 1] + current_board[i    ][j + 1] + current_board[i + 1][j + 1];

                // Game of life
                if (current_board[i][j] == 1) { // Alive cell
                    if (aliveNeighbors < 2 || aliveNeighbors > 3) {
                        next_board[i][j] = 0; // Dies
                    } else {
                        next_board[i][j] = 1; // Lives
                    }
                } else { // Dead cell
                    if (aliveNeighbors == 3) {
                        next_board[i][j] = 1; // Becomes alive
                    } else {
                        next_board[i][j] = 0; // Stays dead
                    }
                }
            }
        }       
        swapBoard(&next_board, &current_board); // Shuffle it in
    }

    const double end_time = get_time();
    const double time_spent = end_time - start_time;

    double max_time;
    MPI_Reduce(&time_spent, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    MPI_Gatherv(&current_board[1][0], sizes_flat[WORLD_RANK], MPI_INT, &full_board[1][0], sizes_flat, displacements_flat, MPI_INT, 0, MPI_COMM_WORLD);

    //if (WORLD_RANK == 0) { printBoard(full_board, N, N, stdout); }

    if (WORLD_RANK == 0) { // Final stuff only done on master rank
        FILE *outputFile = fopen(OUTPUT_FILENAME, "w");
        if (outputFile == NULL) {
            perror("Error opening output file");
            return 1; // Error code
        }
        else {
            printBoard(full_board, N_X, N_X, outputFile);
        }
        fclose(outputFile);

        double end_time = get_time();
        const double time_spent = end_time - start_time;

        printf("EXE: %s, RANKS: %d, SIZE: %d, GENS: %d, SEC: %f, MEM: %d \n", argv[0], WORLD_SIZE, N_X, MAX_GENERATIONS, max_time, sizes_flat[WORLD_RANK]);
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
  
  p = (int *)calloc(P*Q, sizeof(int));
  a = (int **)calloc(P, sizeof(int*));

  if (p == NULL || a == NULL) 
    printf("Error allocating memory\n");

  for (i = 0; i < P; i++)
    a[i] = &p[i*Q];
  
  return a;
}


void swapBoard(int ***board1, int ***board2) {
    int **tempBoard = *board1;
    *board1 = *board2;
    *board2 = tempBoard;
}


int **initializeBoard(int **board, int size_x, int size_y, int seed) {
    if (seed != - 1) {
        srand(seed);
        for (int i = 1; i < size_y + 1; i++) {
            for (int j = 1; j < size_x + 1; j++) {
                board[i][j] = rand() % 2; // Modulus to get 0 or 1
            }
        }
    }
    else {
        for (int i = 1; i < size_y + 1; i++) {
            for (int j = 1; j < size_x + 1; j++) {
                board[i][j] = 1; // Modulus to get 0 or 1
            }
        }
    }


    return board;
}


void printBoard(int **board, int size_x, int size_y, FILE *file) {
    // Print a row of dashes
    for (int j = 0; j <= ((size_x + 1) * 2); j++) {
        fprintf(file, "-");
    }
    fprintf(file, "\n");

    // Make cells wider to make board more square
    for (int i = 0; i < size_y + 2; i++) { // Skip PBC edge
        for (int j = 0; j < size_x + 2; j++) {
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <mpi.h>

// This isn't strictly required I think, header file is somehow useful here?
int **initializeBoard(int **board, int size, int seed);
void printBoard(int **board, int size);
void swapBoard(int **board1, int **board2);
void writeBoardToFile(int **board, int size, FILE *file);
int **allocate_array(int P, int Q);
void free_array(int **a);
double get_time();
int min(int x, int y);


int main(int argc, char *argv[]) {
    double start_time = get_time();

    MPI_Init(&argc, &argv);

    int WORLD_SIZE, WORLD_RANK;
    MPI_Comm_size(MPI_COMM_WORLD, &WORLD_SIZE);
    MPI_Comm_rank(MPI_COMM_WORLD, &WORLD_RANK);

    const int N = atoi(argv[1]); // Board size (no PBC)
    const int MAX_GENERATIONS = atoi(argv[2]); // Max generations
    const int PRINT_ALL = 0; // Old logic, we don't want to output the whole time
    const char *outputFileName = argv[3];
    const int THREADS = 1; // Threads to use
    const int SEED = atoi(argv[4]); // Random Seed

    // MPI
    const int rows_per_process = N / WORLD_SIZE;
    const int start_row = WORLD_RANK * rows_per_process;
    const int final_row = min(start_row + rows_per_process, N);

    int prev_rank = WORLD_RANK - 1;
    int next_rank = WORLD_RANK + 1;

    // Special case for first and lass rank
    if (prev_rank < 0) prev_rank = MPI_PROC_NULL;
    if (next_rank >= WORLD_SIZE) next_rank = MPI_PROC_NULL;

    // Allocate memory for the boards
    int **currentBoard = allocate_array(N + 2, N + 2);
    int **nextBoard = allocate_array(N + 2, N + 2);

    if (WORLD_RANK == 0) {
        currentBoard = initializeBoard(currentBoard, N, SEED);
    }
    
    // Distribute initial array (could be scatter/gather instead)
    for (int i = 0; i < N + 2; i++) {
        // Broadcast each row separately
        MPI_Bcast(currentBoard[i], N + 2, MPI_INT, 0, MPI_COMM_WORLD);
    }
    //printBoard(currentBoard, N); //this ends up a jumbled mess but readable

    // Breaking the loop causes problems
    //int breakLoop = 0;
    //int updated = 0;

    // Game loop
    for (int gen = 0; gen < MAX_GENERATIONS; ++gen) {
        if (prev_rank != MPI_PROC_NULL) {
            MPI_Sendrecv(currentBoard[start_row + 1], N, MPI_INT, prev_rank, 0,
                         currentBoard[start_row], N, MPI_INT, prev_rank, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        if (next_rank != MPI_PROC_NULL) {
            MPI_Sendrecv(currentBoard[final_row - 1], N, MPI_INT, next_rank, 0,
                         currentBoard[final_row], N, MPI_INT, next_rank, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        for (int i = start_row + 1; i < final_row + 1; i++) { // Bounds to only look at board without PBC to avoid issues
            for (int j = 1; j <= N; j++) {
                const int aliveNeighbors  = currentBoard[i - 1][j - 1] + currentBoard[i    ][j - 1] + currentBoard[i + 1][j - 1] +
                                            currentBoard[i - 1][j    ] +                            + currentBoard[i + 1][j    ] +
                                            currentBoard[i - 1][j + 1] + currentBoard[i    ][j + 1] + currentBoard[i + 1][j + 1];

                // Game of life
                if (currentBoard[i][j] == 1) { // Alive cell
                    if (aliveNeighbors < 2 || aliveNeighbors > 3) {
                        nextBoard[i][j] = 0; // Dies
                    } else {
                        nextBoard[i][j] = 1; // Lives
                    }
                } else { // Dead cell
                    if (aliveNeighbors == 3) {
                        nextBoard[i][j] = 1; // Becomes alive
                    } else {
                        nextBoard[i][j] = 0; // Stays dead
                    }
                }
            }
        }
        
        {
            swapBoard(nextBoard, currentBoard); // Shuffle it in

            if (PRINT_ALL) { // Printing this run
                printBoard(currentBoard, N);
            }
        }
    }

    // Collect final array (could be scatter/gather instead)
    for (int i = 0; i < N + 2; i++) {
        // Broadcast each row separately
        int root_rank = min(N / rows_per_process, WORLD_SIZE - 1); //find correct root
        MPI_Bcast(currentBoard[i], N + 2, MPI_INT, root_rank, MPI_COMM_WORLD);
    }
    //printBoard(currentBoard, N); //this ends up a jumbled mess but readable

    if (WORLD_RANK == 0) { // Final stuff only done on master rank
        FILE *outputFile = fopen(outputFileName, "w");
        if (outputFile == NULL) {
            perror("Error opening output file");
            return 1; // Error code
        }
        else {
            writeBoardToFile(currentBoard, N, outputFile);
        }
        fclose(outputFile);

        double end_time = get_time();
        const double time_spent = end_time - start_time;

        printf("EXE: %s, RANKS: %d, SIZE: %d, GENS: %d, SEC: %f \n", argv[0], WORLD_SIZE, N, MAX_GENERATIONS, time_spent);
    }

    free(currentBoard);
    free(nextBoard);

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


int **allocate_array(int P, int Q) {
  int i;
  int *p, **a;
  
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


int **initializeBoard(int **board, int size, int seed) {
    srand(seed);

    for (int j = 0; j < size + 2; j++) {
        board[0][j] = 0; // Top boundary
        board[size + 1][j] = 0; // Bottom boundary
    }

    for (int i = 1; i < size + 1; i++) {
        board[i][0] = 0; // Left boundary
        board[i][size + 1] = 0; // Right boundary
    }

    // Fill in the inner cells
    for (int i = 1; i < size + 1; i++) {
        for (int j = 1; j < size + 1; j++) {
            board[i][j] = rand() % 2; // Modulus to get 0 or 1
        }
    }

    return board;
}


void printBoard(int **board, int size) {
    // Make cells wider to make board more square
    for (int i = 1; i <= size; i++) { // Skip PBC edge
        for (int j = 1; j <= size; j++) {
            printf("%c ", board[i][j] ? 'X' : ' '); // 'X' for alive, ' ' for dead
        }
        printf("\n");
    }

    // Print a row of dashes
    for (int j = 1; j <= (size * 2) - 1; j++) {
        printf("-");
    }
    printf("\n");

}

void writeBoardToFile(int **board, int size, FILE *file) {
    // This could be the same as printBoard, but to a file.  That would make it 
    // easier to maintain logic if I change printBoard.  Consider changing
    for (int i = 1; i <= size; i++) {
        for (int j = 1; j <= size; j++) {
            fprintf(file, "%c ", board[i][j] ? 'X' : ' ');
        }
        fprintf(file, "\n");
    }
    
    for (int j = 1; j <= (size * 2) - 1; j++) {
        fprintf(file, "-");
    }
    fprintf(file, "\n");
}
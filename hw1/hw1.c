#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// This isn't strictly required I think, header file is somehow useful here?
void initializeBoard(int** board, int size);
int updateBoard(int** currentBoard, int** nextBoard, int size);
void printBoard(int** board, int size);
void swapBoard(int** board1, int** board2);


int main(int argc, char *argv[]) {
    const clock_t start_time = clock();

    if (argc < 4) { // Helper like argparse in python
        printf("Usage: %s <board size: int> <max generations: int> <print: int as bool>\n", argv[0]);
        return 1;
    }

    const int N = atoi(argv[1]); // Board size (no PBC)
    const int MAX_GENERATIONS = atoi(argv[2]); // Max generations
    const int PRINT_ALL = (argc > 3) ? atoi(argv[3]) : 0; // Do we print everything?

    // Allocate memory for the boards
    int** currentBoard = (int**) malloc((N+2) * sizeof(int*));
    int** nextBoard = (int**) malloc((N+2) * sizeof(int*));

    for (int i = 0; i < N+2; i++) {
        currentBoard[i] = (int*) malloc((N+2) * sizeof(int));
        nextBoard[i] = (int*) malloc((N+2) * sizeof(int));
    }

    initializeBoard(currentBoard, N);

    // Game loop
    for (int gen = 0; gen < MAX_GENERATIONS; ++gen) {
        const int updated = updateBoard(currentBoard, nextBoard, N); // Find next board
        swapBoard(nextBoard, currentBoard); // Shuffle it in

        if (PRINT_ALL) { // Printing this run
            printBoard(currentBoard, N);
        }

        if (updated) {
          break;
        }
    }

    // Free allocated memory
    for (int i = 0; i < N+2; i++) {
        free(currentBoard[i]);
        free(nextBoard[i]);
    }
    free(currentBoard);
    free(nextBoard);

    const clock_t end_time = clock();
    const double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    printf("EXE: %s SIZE: %d GENS: %d SEC: %f \n", argv[0], N, MAX_GENERATIONS, time_spent);

    return 0;
}

void swapBoard(int** board1, int** board2) {
    int* tempBoard = *board1;
    *board1 = *board2;
    *board2 = tempBoard;
}

void initializeBoard(int** board, int size) {
    srand(time(NULL)); // Do we have to seed in C++?  Not sure

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
}

int updateBoard(int** currentBoard, int** nextBoard, int size) {
    int updated = 0;
    for (int i = 1; i <= size; i++) { // Bounds to only look at board without PBC to avoid issues
        for (int j = 1; j <= size; j++) {
            const int aliveNeighbors  = currentBoard[i - 1][j - 1] + currentBoard[i    ][j - 1] + currentBoard[i + 1][j - 1] +
                                        currentBoard[i - 1][j    ] +                            + currentBoard[i + 1][j    ] +
                                        currentBoard[i - 1][j + 1] + currentBoard[i    ][j + 1] + currentBoard[i + 1][j + 1];

            // Game of life
            if (currentBoard[i][j] == 1) { // Alive cell
                if (aliveNeighbors < 2 || aliveNeighbors > 3) {
                    nextBoard[i][j] = 0; // Dies
                    updated = 1;
                } else {
                    nextBoard[i][j] = 1; // Lives
                }
            } else { // Dead cell
                if (aliveNeighbors == 3) {
                    nextBoard[i][j] = 1; // Becomes alive
                    updated = 1;
                } else {
                    nextBoard[i][j] = 0; // Stays dead
                }
            }
        }
    }
    return updated;
}

void printBoard(int** board, int size) {
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

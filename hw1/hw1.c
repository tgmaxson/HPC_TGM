#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// This isn't strictly required I think, header file is somehow useful here?
void initializeBoard(int** board, int size);
void updateBoard(int** currentBoard, int** nextBoard, int size);
void printBoard(int** board, int size);
void copyBoard(int** source, int** destination, int size);


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
        updateBoard(currentBoard, nextBoard, N); // Find next board
        copyBoard(nextBoard, currentBoard, N); // Shuffle it in

        if (PRINT_ALL) { // Printing this run
            printBoard(currentBoard, N);
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

    printf("EXE: %s SIZE: %d GENS: %d SEC: %f.\n", argv[0], N, MAX_GENERATIONS, time_spent);

    return 0;
}

void copyBoard(int** source, int** destination, int size) {
    for (int i = 0; i < size+2; i++) {
        memcpy(destination[i], source[i], (size+2) * sizeof(int));
    }
}

void initializeBoard(int** board, int size) {
    srand(time(NULL)); // Do we have to seed in C++?  Not sure

    for (int i = 0; i < size + 2; i++) {
        for (int j = 0; j < size + 2; j++) {
            if (i == 0 || i == size + 1 || j == 0 || j == size + 1) { // PBC edge cells
                board[i][j] = 0; // PBC should be dead
            } else {
                board[i][j] = rand() % 2; // Modulus to get 0 or 1
            }
        }
    }
}

void updateBoard(int** currentBoard, int** nextBoard, int size) {
    for (int i = 1; i <= size; i++) { // Bounds to only look at board without PBC to avoid issues
        for (int j = 1; j <= size; j++) {
            int aliveNeighbors = 0;

            for (int x = -1; x <= 1; x++) { // Get number of alive neighbors
                for (int y = -1; y <= 1; y++) {
                    if (x == 0 && y == 0) continue; // Skip the cell itself
                    aliveNeighbors += currentBoard[i + x][j + y];
                }
            }

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
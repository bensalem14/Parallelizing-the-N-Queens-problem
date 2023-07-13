#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <omp.h>
#include <mpi.h>
#include <stdbool.h>

static const uint32_t default_n = 16;
uint32_t taille = 0;
int ind, rc, rank, num_procs;






#define NUM_THREADS	4

typedef struct data{
    int numThread;
    uint32_t startIteration;
    uint32_t endIteration;
    uint32_t val;

    uint32_t *queen_positions;  // Store queen positions on the board
    uint32_t *column;           // Store available column moves/attacks
    uint32_t *diagonal_up;      // Store available diagonal moves/attacks
    uint32_t *diagonal_down;
    uint32_t column_j;          // Stores column to place the next queen in

    uint64_t placements;        // Tracks total number queen placements
    uint64_t solutions;         // Tracks number of solutions
}data;

// An abstract representation of an NxN chess board to tracking open positions
struct chess_board {
  uint32_t n_size;            // Number of queens on the NxN chess board
  uint32_t *queen_positions;  // Store queen positions on the board
  uint32_t *column;           // Store available column moves/attacks
  uint32_t *diagonal_up;      // Store available diagonal moves/attacks
  uint32_t *diagonal_down;
  uint32_t column_j;          // Stores column to place the next queen in
  uint64_t placements;        // Tracks total number queen placements
  uint64_t solutions;         // Tracks number of solutions
};
static struct chess_board *board;

static void initialize_board(const uint32_t n_queens) {
  if (n_queens < 1) {
    fprintf(stderr, "The number of queens must be greater than 0.\n");
    exit(EXIT_SUCCESS);
  }

  // Dynamically allocate memory for chessboard struct
  board = malloc(sizeof(struct chess_board));
  if (board == NULL) {
    fprintf(stderr, "Memory allocation failed for chess board.\n");
    exit(EXIT_FAILURE);
  }

  // Initialize the chess board parameters
  board->n_size = n_queens;
  board->placements = 0;
  board->solutions = 0;
}
// Handles dynamic memory allocation of the arrays and sets initial values
data * initialize_threadStruct(  ) {
  // Dynamically allocate memory for chessboard struct
  data* borad_infos = malloc(sizeof(struct data));
  if (borad_infos == NULL) {
    fprintf(stderr, "Memory allocation failed for borad_infos.\n");
    exit(EXIT_FAILURE);
  }

  // Dynamically allocate memory for chessboard arrays that track positions
  const uint32_t diagonal_size = 2 * board->n_size - 1;
  const uint32_t total_size = 2 * (board->n_size + diagonal_size);
  borad_infos->queen_positions = malloc(sizeof(uint32_t) * total_size);
  if (borad_infos->queen_positions == NULL) {
    fprintf(stderr, "Memory allocation failed for the borad_infos arrays.\n");
    free(borad_infos);
    exit(EXIT_FAILURE);
  }
  borad_infos->column = &borad_infos->queen_positions[board->n_size];
  borad_infos->diagonal_up = &borad_infos->column[board->n_size];
  borad_infos->diagonal_down = &borad_infos->diagonal_up[diagonal_size];

  // Initialize the chess board parameters
  for (uint32_t i = 0; i < board->n_size; ++i) {
    borad_infos->queen_positions[i] = 0;
  }
  for (uint32_t i = board->n_size; i < total_size; ++i) {
    // Initializes values for column, diagonal_up, and diagonal_down
    borad_infos->queen_positions[i] = 1;
  }
  borad_infos->column_j = 0;
 // printf("dan init et borad_infos->column_j=%d\n",borad_infos->column_j);
  borad_infos->placements = 0;
  borad_infos->solutions = 0;
  borad_infos->val = 0;
  return(borad_infos);
}

// Frees the dynamically allocated memory for the chess board structure
static void smash_board() {
  free(board);
}
void smash_threadStruct(data *borad_infos) {
  free(borad_infos->queen_positions);
  free(borad_infos);
}

// Check if a queen can be placed in at row 'i' of the current column
uint32_t square_is_free(const uint32_t row_i, data *borad_infos) {
    return borad_infos->column[row_i] &
        borad_infos->diagonal_up[(board->n_size - 1) + (borad_infos->column_j - row_i)] &
        borad_infos->diagonal_down[borad_infos->column_j + row_i];
}

// Place a queen on the chess board at row 'i' of the current column
void set_queen(const uint32_t row_i, data *borad_infos) 
{
  borad_infos->queen_positions[borad_infos->column_j] = row_i;
  borad_infos->column[row_i] = 0;
  borad_infos->diagonal_up[(board->n_size - 1) + (borad_infos->column_j - row_i)] = 0;
  borad_infos->diagonal_down[borad_infos->column_j + row_i] = 0;
  ++borad_infos->column_j;
  ++borad_infos->placements;
}

// Removes a queen from the NxN chess board in the given column to backtrack
void remove_queen(const uint32_t row_i,  data *borad_infos) {
  --borad_infos->column_j;
  borad_infos->diagonal_down[borad_infos->column_j + row_i] = 1;
  borad_infos->diagonal_up[(board->n_size - 1) + (borad_infos->column_j - row_i)] = 1;
  borad_infos->column[row_i] = 1;
}

// Prints the number of queen placements and solutions for the NxN chess board
static void print_counts() {
  // The next line fixes double-counting when solving the 1-queen problem
  const uint64_t solution_count = board->n_size == 1 ? 1 : board->solutions;
  const char const output[] = "The %u-Queens problem required %lu queen "
                            "placements to find all %lu solutions\n";
  fprintf(stdout, output, board->n_size, board->placements, solution_count);
}

// Recursive function for finding valid queen placements on the chess board


void place_next_queen(data* pntr) {
    const uint32_t middle = pntr->column_j ? board->n_size : board->n_size >> 1;

  int limit = pntr->val;
    for (uint32_t row_i = 0; row_i <  limit; row_i++) 
    { 
            if (square_is_free(row_i, pntr)) 
            {  
                set_queen(row_i, pntr);
                if (pntr->column_j == board->n_size) {
                    pntr->solutions += 2;

                  
                } 
                else 
                  {
                    pntr->val = board->n_size;
                    place_next_queen( pntr );
                  } 

                remove_queen(row_i, pntr);
            }
    }

  
}




int main(int argc, char *argv[]) {
 double start_time, end_time;
  static const uint32_t default_n = 16;
    int *status, ind, rc;
    uint32_t taille = 0;
    const uint32_t n_queens = (argc != 1) ? (uint32_t)atoi(argv[1]) : default_n;
    initialize_board(n_queens);
////////


 const uint32_t row_boundary = (n_queens >> 1) + (n_queens & 1);
  const uint32_t middle = board->n_size >> 1;
////////////


data *thread_infos[NUM_THREADS], *structThread, *adress;
taille = row_boundary/NUM_THREADS;

///////////

int num_processes, rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

start_time = MPI_Wtime();


structThread = initialize_threadStruct();

structThread->numThread = rank;
structThread->val = row_boundary;
 thread_infos[rank]  = structThread;
//  printf("rank is %d\n",rank);

// MPI_Barrier(MPI_COMM_WORLD);
/////

int chunk_size = row_boundary / num_processes;
int start = rank * chunk_size;
int end = (rank == num_processes - 1) ? row_boundary : start + chunk_size;
printf("start %d end %d rank %d\n",start,end,rank);
for (int i = start; i < end; i++) 
{
    int threadNum = rank;


    set_queen(i, thread_infos[threadNum]);

    if (i != middle) {
        thread_infos[threadNum]->val = board->n_size;
    } else {
        thread_infos[threadNum]->val = (board->n_size >> 1);
    }

    place_next_queen(thread_infos[threadNum]);
    remove_queen(i, thread_infos[threadNum]);

    printf("Iteration i = %d, thread: %d, et nb solution %d\n", i , rank, thread_infos[threadNum]->solutions);
}
MPI_Barrier(MPI_COMM_WORLD);



int sum = 0;
for (int i = 0; i < NUM_THREADS; i++) 
{
    if (rank == i) 
    {
        printf("process: %d, et nb solution %d\n", i, thread_infos[i]->solutions);
    }
    // MPI_Barrier(MPI_COMM_WORLD);
}

MPI_Reduce(&thread_infos[rank]->solutions, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
if (rank == 0) 
{
    printf("SOLUTIONS %d\n", sum);
}

// MPI_Barrier(MPI_COMM_WORLD);

end_time = MPI_Wtime();
 if (rank == 0) {
        printf("OpenMPI time = %f seconds\n", end_time - start_time);
  }
MPI_Finalize();

  return EXIT_SUCCESS;
}
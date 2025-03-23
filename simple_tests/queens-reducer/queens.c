/**
 * Copyright (c) 2020 MIT License by 6.172 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/

#include <cilk/cilk.h>

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "../fasttime.h"
#include "board.h"

// board.h defines N, board_t, row_t, and helper functions.

#ifdef SERIAL
  #define cilk_for for
  #define cilk_spawn
  #define cilk_scope
#endif

const unsigned I = 1UL << 16;  // number of iterations
int MAX_SOLUTIONS = -1;

// Feel free to make this 0.
const unsigned TO_PRINT = 3;  // number of sample solutions to print

const row_t BITMASK = 0b11111111U;  // 8 "1"s

void boardList_identity(void *value) {
  *(BoardList *)value = (BoardList) {.head = NULL, .tail = NULL, .size = 0}; // Cast to BoardList*, de-reference, assign
}

void boardList_reduce(void *left, void *right) {
  BoardList *left_ptr = (BoardList*)left;    // Cast to BoardList*
  BoardList *right_ptr = (BoardList*)right;

  if(right_ptr->head == NULL) { // Right is empty, no need to add
    return;
  }

  if(left_ptr->head == NULL) { // If left is empty, then left points to right's head
    left_ptr->head = right_ptr->head;
  } 
  else { // Simply connect left's tail to right's head
    left_ptr->tail->next = right_ptr->head;
  }
  left_ptr->tail = right_ptr->tail;
  
  left_ptr->size += right_ptr->size; // Update left's size
}

typedef BoardList cilk_reducer(boardList_identity, boardList_reduce) BoardListReducer;

void queens(BoardListReducer* board_list,
            board_t cur_board,
            row_t row,
            row_t down,
            row_t left,
            row_t right) {
  bool verbose = MAX_SOLUTIONS > 0;
  if (MAX_SOLUTIONS > 0 && board_list->size >= MAX_SOLUTIONS)
  {
    printf("MAX_SOLUTIONS=%d found. Exiting.\n", MAX_SOLUTIONS);
    cilk_sync;
    exit(0);
  }
    
  if (row == N) {
    // A solution to 8 queens!
    BoardList *board_list_ptr = &(*board_list); // De-reference to get thread-local BoardList view
    append_node(board_list_ptr, cur_board);    
    if (verbose) printf("Found solution!\n");
  } else {
    int open_cols_bitmap = BITMASK & ~(down | left | right);

    while (open_cols_bitmap != 0) {
      int bit = -open_cols_bitmap & open_cols_bitmap;
      int col = log2(bit);
      open_cols_bitmap ^= bit;

      if (verbose) printf("Before queens: row %d, col %d, bit %3d, open_cols_bitmap %3d\n",
            row, col, bit, open_cols_bitmap);

      // Recurse! This can be parallelized.
      cilk_spawn queens(board_list, cur_board | board_bitmask(row, col), row + 1,
             down | bit, (left | bit) << 1, (right | bit) >> 1);
      
      if (verbose) printf("After  queens: row %d, col %d, bit %3d, open_cols_bitmap %3d\n", 
             row, col, bit, open_cols_bitmap);
    }
    cilk_sync;
  }
  if (verbose) printf("(Returning from queens)\n");
}

int run_queens(bool verbose) {
  BoardListReducer board_list = {.head = NULL, .tail = NULL, .size = 0};

  queens(__builtin_addressof(board_list), (board_t)0, 0, 0, 0, 0);
  int num_solutions = board_list.size;

  if (verbose) {
    // Print the first few solutions to check correctness.
    BoardNode* cur_node = board_list.head;
    int num_printed = 0;
    while (cur_node != NULL && num_printed < TO_PRINT) {
      printf("Solution # %d / %d\n", num_printed + 1, num_solutions);
      print_board(cur_node->board);
      cur_node = cur_node->next;
      num_printed++;
    }
  }
  BoardList *board_list_ptr = (BoardList*)__builtin_addressof(board_list);
  delete_nodes(board_list_ptr);
  return num_solutions;
}

int main(int argc, char* argv[]) {

  if (argc > 1) {
    MAX_SOLUTIONS = atoi(argv[1]);
    printf("Using MAX_SOLUTIONS=%d for debugging.\n", MAX_SOLUTIONS);
  }

  int num_solutions = run_queens(true);

#ifdef CILKSAN
  (void)num_solutions;
  printf("Only ran once because CILKSAN is enabled.\n");
#else
  fasttime_t start = gettime();
  for (int i = 0; i < I; i++) {
    run_queens(false);
  }
  fasttime_t stop = gettime();

  printf("Elapsed execution time: %f sec; N: %d, I: %d, num_solutions: %d\n",
         tdiff_sec(start, stop), N, I, num_solutions);
#endif

  return 0;
}

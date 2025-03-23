# OpenCilk Test Code

This repository contains codes that use OpenCilk to exploit parallelization in CPU. These codes were used to submit for assignments in a course called Performance Engineering of Software Systems (EEC 289Q, Fall 2023) at University of California, Davis.

To build an exectuable, simply perform `make` in each test case directory. (like `simple_tests/fib`, `simple_tests/qsort`, etc.) Here is a brief description of each test case:

## Simple Tests
- `fib`: Fibonacci Sequence. Usage: `fib N`.
- `qsort`: Quicksort. Usage: `qsort N NUM_RUNS`.
- `queens`: N Queens problem, placing N queens in N x N chessboard such that no queen attacks another. (i.e., no two queens are found in any row, column, or diagonal). For a scope of assignment, the program only works with `N = 8`. Usage: `queens`
- `queens-reducer`: N Queens problem but using Cilk reducers. Usage: `queens`

## Quadtree
This project involves performing a quadtree-based collision detection in a 2D virtual enviornment called screensaver. As a dependency, this project requires `X11` library. Sample input files can be found in `input` directory. Usage: `screensaver NUM_FRAMES INPUT_FILE`

## Crossword
This project involves a crossword compiler that takes an empty crossword grid and fill it with given words. There are several command-line options, listed below. 
- `-m #`. The wordlist has a score for every word, with higher scores indicating better words. The scores range from 10-50. This command line option indicates "only consider words with scores of # or higher". Setting a high minimum score (e.g., `-m 50`) considers fewer words than a lower minimum score. It thus constrains possible fill options.
- `-g gridfile`. `gridfile` is a filename that contains an (empty) crossword grid.
- `-w wordlist`. `wordlist` is a filename that contains a wordlist of allowed words.
- `-s r,c,DIR,word`. This specifies a seed word (an entry that is already in the grid when you start filling), to be placed in the grid at location (r,c) in the direction `DIR`. `DIR` is either the character a (across) or d (down). `word` is the seed word. Make sure the size of word is the size of the entry in the grid. You can specify as many seeds as you wish; you must specify at least one.
- `-o outputfile`. Write the final output grid to `outputfile`.
### Sample command line
```
./xword -m 50 -g grids/5x5blankgrid.txt -w wordlists/spreadthewordlist.txt -s 0,0,a,scale
```

For any questions, please contact mgcho@ucdavis.edu
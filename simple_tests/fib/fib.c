#include <stdio.h>
#include <stdlib.h>

#include <cilk/cilk.h>

int fib(int n) {
  if (n < 10) {
    int num0 = 0, num1 = 1, sum = n;

    for (int i = 1; i < n; i++) {
      sum = num0 + num1;
      num0 = num1;
      num1 = sum;
    }

    return sum;
  }

  int x, y;
  
  // Serial Projection
  #ifdef SERIAL
    x = fib(n - 1);
    y = fib(n - 2);
  #else
    // Cilk Implementation
    cilk_scope {
      x = cilk_spawn fib(n - 1);
      y = fib(n - 2);
    }
  #endif

  return x + y;
}

int main(int argc, char* argv[]) {
  int n = 10;

  if (argc > 1) {
    n = atoi(argv[1]);
  }

  int result = fib(n);
  printf("fib(%d)=%d\n", n, result);

  return 0;
}

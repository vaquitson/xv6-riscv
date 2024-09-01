#include "user.h"

int main(void){
  printf("cur proces: %d\n", getpid());
  printf("ancestor 0 proces: %d\n", getancestor(0));
  printf("ancestor 1 proces: %d\n", getancestor(1));
  printf("ancestor 2 proces: %d\n", getancestor(2));
  printf("ancestor 3 proces: %d\n", getancestor(3));
  printf("ancestor 4 proces: %d\n", getancestor(4));
  printf("ancestor 5 proces: %d\n", getancestor(5));
  printf("ancestor 6 proces: %d\n", getancestor(6));
}

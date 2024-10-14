#include "user.h"

int main(void){
  printf("super parent: %d\n", getpid());
  int nProc = 4;

  for (int i = 0; i < nProc; i++){
    sleep(1);
    int pid = fork();
    if (pid < 0){
      printf("fork faild");
      return 1;
    } else if (pid == 0){
      int procesPid = getpid();
      printf("process created: %d\n", procesPid);
      sleep(2);
      printf("process finish: %d\n", procesPid);
      break;
    }
  }

  for (int j = 0; j < nProc; j++){
    wait(&j);
  }

}

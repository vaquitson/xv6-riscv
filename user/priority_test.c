#include "user.h"

int numeroDeProcesos = 25;

int main(void){
  printf("numero de procesos: %d\n", numeroDeProcesos);
  printf("super parent: %d\n", getpid());

  for (int i = 0; i < numeroDeProcesos; i++){
    sleep(1);
    int pid = fork();
    if (pid == 0){
      int procesPid = getpid();
      printf("process created: %d\n", procesPid);
      sleep(3);
      for (int j = 0; j < 1000000*i; j++){
        void *ptr = malloc(30); //perder tiempo en cpu
        free(ptr);
      }
      printf("process finish: %d\n", procesPid);
      break;

    } else if (pid < 0){
      printf("fork faild");
      return 1;
    }
  }

  for (int k = 0; k < numeroDeProcesos; k++){
    wait(&k);
  }

}

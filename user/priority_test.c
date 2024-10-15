#include "user.h"

int numeroDeProcesos = 25;

int main(void){
  printf("numero de procesos: %d\n", numeroDeProcesos);
  printf("super parent: %d\n", getpid());

  for (int count = 0; count < numeroDeProcesos; count++){
    sleep(1);
    int pid = fork();
    if (pid == 0){
      int procesPid = getpid();
      printf("process created: %d\n", procesPid);
      sleep(3);
      printf("finish: %d\n", procesPid);
      break;

    } else if (pid < 0){
      printf("fork faild");
      return 1;
    }
  } 

}

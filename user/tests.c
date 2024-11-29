#include "../kernel/fcntl.h"
#include "../user/user.h"

#define NO_PERMISION 0
#define READ_O 1
#define WRITE_O 2
#define READ_WRITE 3
#define INMUTABLE 4

int main(){
  char buffer[10];
  char buffer2[10];

  int fd = open("hola.txt", (O_CREATE | O_RDWR));
  printf("archivo creado, fd: %d\n\n", fd);
  
  printf("resultado escritura %d\n\n", write(fd, "hola", 5));

  close(fd);
  printf("archivo cerrado\n\n");


  printf("rc de chmode %d\n\n", chmode("./hola.txt", READ_O));
  
  fd = open("hola.txt", O_WRONLY);

  printf("intentando abrir archivo solo lectura como ESCRITURA, fd: %d\n\n", fd);

  fd = open("hola.txt", O_RDONLY);
  printf("intentando abrir archivo solo lectura como SOLO LECTURA, fd: %d\n", fd);

  read(fd, buffer, 5);
  printf("resultado lectura %s\n", buffer);

  close(fd);

  printf("cambiando permisos a INMUTABLE\n");
  printf("rc de chmode %d\n\n", chmode("./hola.txt", INMUTABLE));

  printf("cambiando nuevamente los permisos, rc: %d\n\n", chmode("./hola.txt", READ_WRITE));
  
  fd = open("hola.txt", O_RDWR);
  printf("abriendo archvio como LECTURA Y ESCRITURA, fd: %d\n", fd);

  fd = open("hola.txt", O_RDONLY);
  printf("intentando abrir archivo solo lectura como SOLO LECTURA, fd: %d\n", fd);

  read(fd, buffer2, 5);
  printf("resultado lectura %s\n", buffer);

 
  close(fd);
  return 0;
}

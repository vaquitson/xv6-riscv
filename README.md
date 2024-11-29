Benjamín Liberona
# Primera parte

lo primero que haremos sera agregar este nuevo campo de permisos en el inode.

agregamos el campo **protection** en el el archivo **kernel/file.h** 
```c
#define NO_PERMISION 0
#define READ_O 1
#define WRITE_O 2
#define READ_WRITE 3

struct inode {
  uint dev;           // Device number
  uint inum;          // Inode number
  int ref;            // Reference count
  struct sleeplock lock; // protects everything below here
  int valid;          // inode has been read from disk?
  
  int prtotection;    // bits for protection of the file

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT+1];
};
```

también se agregaron definiciones
para que sea mas legible el código

# Segunda parte
lo siguiente que haremos sera modificar la función **create**, esto con el objetivo de poder crear archivos con el  nuevo campo inicializado en 3 

```c
static struct inode*
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
    return 0;

  ilock(dp);

  if((ip = dirlookup(dp, name, 0)) != 0){
    iunlockput(dp);
    ilock(ip);
    if(type == T_FILE && (ip->type == T_FILE || ip->type == T_DEVICE))
      return ip;
    iunlockput(ip);
    return 0;
  }

  if((ip = ialloc(dp->dev, type)) == 0){
    iunlockput(dp);
    return 0;
  }

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  ip->prtotection = READ_WRITE; // inicializamos la variable en 3
  iupdate(ip);
  printf("ip_with_protection");

  if(type == T_DIR){  // Create . and .. entries.
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      goto fail;
  }

  if(dirlink(dp, name, ip->inum) < 0)
    goto fail;

  if(type == T_DIR){
    // now that success is guaranteed:
    dp->nlink++;  // for ".."
    iupdate(dp);
  }

  iunlockput(dp);

  return ip;

 fail:
  // something went wrong. de-allocate ip.
  ip->nlink = 0;
  iupdate(ip);
  iunlockput(ip);
  iunlockput(dp);
  return 0;
}
```

en esta parte agregamos la linea   
**ip->prtotection = READ_WRITE;**
para poder asignar la variable a lectura y escritura

# Tercera parte
ahora lo que aremos sera que cuando intentemos abrir el archivo, este validara si los permisos son correctos o no.
Para esto modificamos la función **sys_open** que se encuentra en **kernel/sysfile.c**

```c
uint64
sys_open(void)
{
  char path[MAXPATH];
  int fd, omode;
  struct file *f;
  struct inode *ip;
  int n;

  argint(1, &omode);
  if((n = argstr(0, path, MAXPATH)) < 0)
    return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if(ip->type == T_DEVICE && (ip->major < 0 || ip->major >= NDEV)){
    iunlockput(ip);
    end_op();
    return -1;
  }


  // si el archvio tiene algun permiso de escritura lanzamos error
  if (ip->prtotection == READ_O && (omode & (O_WRONLY | O_RDWR | O_TRUNC)) > 0){
    iunlockput(ip);
    end_op();
    return -1;
  }

  // si el archivo tiene algun permiso de lectura lanzamos un error
  if (ip->prtotection == WRITE_O && (omode & (O_RDONLY | O_RDWR)) > 0){
    iunlockput(ip);
    end_op();
    return -1;
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }


  if(ip->type == T_DEVICE){
    f->type = FD_DEVICE;
    f->major = ip->major;
  } else {
    f->type = FD_INODE;
    f->off = 0;
  }
  f->ip = ip;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

  if((omode & O_TRUNC) && ip->type == T_FILE){
    itrunc(ip);
  }

  iunlock(ip);
  end_op();

  return fd;
}

```

Aquí agregamos dos validaciones, la primera es que si el archivo es de solo lectura, lanzaremos un error si es que intenta ser abierto en modo  **O_WRONLY, O_RDWR, y/o O_TRUNC** .
La segunda corresponde a si el archivo es de solo escritura, lanzaremos un error en caso de que se abra en **O_RDONLY y/o O_RDWR** .

Notar que es importante que la  validación ocurra antes de que se asigne un espacio para el archivo, de lo contrario, quedan descriptores de archivo abiertos sin usar.

esto lo hacemos en las siguientes lineas.

```c
  // si el archvio tiene algun permiso de escritura lanzamos error
  if (ip->prtotection == READ_O && (omode & (O_WRONLY | O_RDWR | O_TRUNC)) > 0){
    printf("hola estamos en proteccion de READ_O\n");
    iunlockput(ip);
    end_op();
    return -1;
  }

  // si el archivo tiene algun permiso de lectura lanzamos un error
  if (ip->prtotection == WRITE_O && (omode & (O_RDONLY | O_RDWR)) > 0){
    printf("hola estamos en proteccion de WRITE_O\n");
    iunlockput(ip);
    end_op();
    return -1;
  }
```

La idea es que devolveremos -1 en caso de error, 
# Paso 5
Lo siguiente que haremos, sera crear la syscall **chmode**, la cual tendrá por objetivo cambiar los permisos de un archivo.
Esta esta definida en el archivo **kernel/sysfile.c** . No entraremos en mas detalle de todos los archivos que hay que modificar para hacer un syscall, ya que fueron detallados en la tarea 1

```c
uint64 sys_chmode(void){

// definimos las direcciones de memoria para los argumentos de la funcion
  char path[MAXPATH];
  int permiso;
  struct inode *ip;
  int n;
  
// asignamos los argumentos de la funcion
  if ((n = argstr(0, path, MAXPATH)) < 0){
    return -1;
  }

  argint(1, &permiso);
  
  begin_op();
  // buscamos el inode correspondiente
  if ((ip = namei(path)) == 0){
    end_op();
    return -1;
  }

// obtenemos el lock del inode y berificamos si es un archivo normal o algo como un directorio o un dispositivo
  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  };

  if (ip->type == T_DEVICE){
    iunlockput(ip);
    end_op();
    return -1;
  }

// si el permiso es un numero mas grande que los permisos que tenemos retornamos nulo
  if (permiso < 0 || permiso > 4){
    return -1;
  }

  ip->prtotection = permiso;
  // cambiamos la proteccion 
  printf("proteccion cambiada a %d\n", ip->prtotection);
  iunlockput(ip);
  end_op();
  return 0;definicion
}
```

la parte **printf("proteccion cambiada a %d\n", ip->prtotection);** es opcional y se puede borrar, solo esta ahí para poder ver el comportamiento mas fácil

La función en éxito, devolverá 0 y en error devolverá un 1

# Parte 6
Agregaremos ahora la nueva protección de **INMUTABLE**, la cual le dará solo permisos de lectura, pero ya no se podrá cambiar su protección.
Para esto lo primero que haremos sera agregar una nueva definición en el archivo **kernel/sysfile.c**.

```c
#define NO_PERMISION 0
#define READ_O 1
#define WRITE_O 2
#define READ_WRITE 3
#define INMUTABLE

struct inode {
  uint dev;           // Device number
  uint inum;          // Inode number
  int ref;            // Reference count
  struct sleeplock lock; // protects everything below here
  int valid;          // inode has been read from disk?
  
  int prtotection;    // bits for protection of the file

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT+1];
};
```

Tambien modificaremos la syscall **chmode** (kernel/sysfile.c) , la cual ahora fallara en caso de que se quiera cambiar la protección de un archivo que es inmutable

```c
uint64 sys_chmode(void){
  char path[MAXPATH];
  int permiso;
  struct inode *ip;
  int n;

  if ((n = argstr(0, path, MAXPATH)) < 0){
    return -1;
  }

  argint(1, &permiso);
  
  begin_op();
  if ((ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  };

  if (ip->type == T_DEVICE){
    iunlockput(ip);
    end_op();
    return -1;
  }

  if (ip->prtotection == INMUTABLE){
    iunlockput(ip);
    end_op();
    return -1;
  }

  if (permiso < 0 || permiso > 4){
    iunlockput(ip);
    end_op();
    return -1;
  }

  ip->prtotection = permiso;
  // string opcional para el teste de la proteccion, se puede borrar
  printf("proteccion cambiada a %d\n", ip->prtotection);
  iunlockput(ip);
  end_op();
  return 0;
}
```

Para esto agregamos la siguiente linea

```c
  if (ip->prtotection == INMUTABLE){
    iunlockput(ip);
    end_op();
    return -1;
  }
```

También debemos modificar la función **open** ( kernel/sysfile.c) para que si un archivo es inmutable, solo puedan realizarse acciones de lectura sobre él

```c
uint64
sys_open(void)
{
  char path[MAXPATH];
  int fd, omode;
  struct file *f;
  struct inode *ip;
  int n;

  argint(1, &omode);
  if((n = argstr(0, path, MAXPATH)) < 0)
    return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if(ip->type == T_DEVICE && (ip->major < 0 || ip->major >= NDEV)){
    iunlockput(ip);
    end_op();
    return -1;
  }


  // si el archvio tiene algun permiso de escritura lanzamos error
  if (ip->prtotection == READ_O && (omode & (O_WRONLY | O_RDWR | O_TRUNC)) > 0){
    iunlockput(ip);
    end_op();
    return -1;
  }

  if (ip->prtotection == INMUTABLE && (omode & (O_WRONLY | O_RDWR | O_TRUNC)) > 0){
    iunlockput(ip);
    end_op();
    return -1;
  }

  // si el archivo tiene algun permiso de lectura lanzamos un error
  if (ip->prtotection == WRITE_O && (omode & (O_RDONLY | O_RDWR)) > 0){
    iunlockput(ip);
    end_op();
    return -1;
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f){
      fileclose(f);
    }
    iunlockput(ip);
    end_op();
    return -1;
  }


  if(ip->type == T_DEVICE){
    f->type = FD_DEVICE;
    f->major = ip->major;
  } else {
    f->type = FD_INODE;
    f->off = 0;
  }
  f->ip = ip;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

  if((omode & O_TRUNC) && ip->type == T_FILE){
    itrunc(ip);
  }

  iunlock(ip);
  end_op();

  return fd;
}
```

Aquí agregamos otra verificación de permisos
```c
  if (ip->prtotection == INMUTABLE && (omode & (O_WRONLY | O_RDWR | O_TRUNC)) > 0){
    iunlockput(ip);
    end_op();
    return -1;
  }
```
# parte 7
Ahora realizaremos las pruebas. Estas están en el archivo **user/tests.c**.
```c
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
```

Como podemos ver, aquí probamos varias cosas.
* la primera es que podamos escribir el archivo recién creado y mostrando en terminal su descriptor de archivo a la hora de crearse
* luego probamos que funcione **chmode** mostrando en pantalla su codigo de retorno, el cual es 0 en éxito y -1 en fallo
* luego comprábamos que el permiso funcione intentando abrir el archivo que es solo lectura como de escritura y mostrando el codigo de retorno de la función **open**
* adicionalmente, agregamos abrir el archivo con los permisos adecuados y leer lo que hay en el para comprobar primero que podemos leer un archivo de solo lectura y segundo comprobar que el descriptor de archivo es el correcto
* luego probaremos agregar este modo inmutable y ver si lo podemos cambiar
* luego veremos si con este nuevo modo podemos seguir leyendo el archivo

# Posdata
Como el informe es hecho a medida que se va construyendo el código, es posible que hayan algunas sutiles diferencias.
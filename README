**Benjamin Liberona**
Queremos agregar una una politica de scheduling similar a MLFQ
## Añadir atributo de prioridad.
Aquí lo primero que haremos, sera agregar un atributo priority y un atributo boost em la estructura **proc** que se encuentra en el archivo **proc.h**.
Posteriormente, inicializaremos la prioridad en 0 y el boost en 1. Esto lo haremos en la función **allocproc** del archivo **proc.c**, la cual esta  encargada de alocar el procesos en el array de procesos, alocar las paginas de memoria, etc. De esta forma, aprovecharemos este momento para inicializar estas variables. 

## Ordenar La Ejecución
para saber en que orden debemos ejecutar los procesos, debemos ordenarlos. Para esto tenderemos una variable global, la cual se llama **curPrio** la cual esta en el archivo proc.c. Esta variable, indica cual es la prioridad que estamos buscando actualmente. Como la estructura del scheduler es de doble for, nos aseguramos de primero buscar las prioridades mas bajas a las mas altas. Para evitar el problema de que un proceso podría alocarse en mitad de la búsqueda, cada vez que un proceso se aloque, volveremos la variable **curPrio** a 0.

## Cambiar las prioridades al alocarse un nuevo proceso

Un cambio importante es el de la función allocproc, en el cual hay que agregar la lógica de  priority += boost. 

Aprovechamos el momento de buscar los espacios sin usar para aumentar la prioridad de todos los procesos .
Una vez encontrado un espacio vació, lo guardamos con la variable p.  Luego seguimos iterando para subir todas las prioridades.
Finalmente si encoramos un espacio vació, conseguimos el lock de ese proceso y vamos al **goto** donde se termina de allocar.

```c
static struct proc*
allocproc(void)
{
  struct proc *p_;
  struct proc *p;

  int foundFlag = 0;

  for(p_ = proc; p_ < &proc[NPROC]; p_++) {
    acquire(&p_->lock);

    if (p_->state == RUNNABLE || p_->state == RUNNING || p_->state == SLEEPING){
      // actualizar prioridades de procesos RUNNABLE cada vez que entra
      // un nuevo proceso
      p_->priority += p_->boost;
      if (p_->priority == 9){
        p_->boost = -1;
      } else if (p_->priority == 0){
        p_->boost = 1;
      }
    }

    if(p_->state == UNUSED) {
      // consegimos una direccion de memoria sin usar
      p = p_;
      foundFlag = 1;
      //goto found;
    }
    release(&p_->lock);
  }

  if (foundFlag == 1){
    acquire(&p->lock);
    goto found;
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  p->priority = 0; // inicializar priority
  p->boost = 1;    // inicializar boost
  
  i = 0;
  foundMatchingPriority = 1 

  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  return p;
}
```

En esta parte, no solo nos aseguramos de subir la prioridad de los procesos  que estén listos para correr, sino también,  los que estén durmiendo y los que estén corriendo, esto para evitar el problema de que los procesos dejen de avanzar porque se crean ZOMBIES que no recojan su valor debido a que los procesos padres tienen prioridad muy baja.
## Modificar el scheduler
ahora encontramos uno de los problemas mas grandes y es que hacer con los procesos que estén durmiendo.  

```c
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  int found;
  for(;;){
    // The most recent process to run may have had interrupts
    // turned off; enable them to avoid a deadlock if all
    // processes are waiting.
    intr_on();

    found = 0;

    for (curPrio = 0; curPrio < 10; curPrio++){
      foundMatchingPriority = 0;
      for(p = proc; p < &proc[NPROC]; p++){ 
        acquire(&p->lock);
        if(p->state == RUNNABLE) {
          if (p->priority == curPrio){
            // Switch to chosen process.  It is the process's job
            // to release its lock and then reacquire it
            // before jumping back to us.
            p->state = RUNNING;
            c->proc = p;
            // print the prio when running
            printf("proces: %d prio: %d boost: %d\n", p->pid, p->priority, p->boost);
            swtch(&c->context, &p->context);
            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc = 0;

            foundMatchingPriority = 1;
          }
          found = 1;
        // control de seguridad
        } else if (p->priority < curPrio && p->state == RUNNABLE){
          curPrio = 0;
          p = proc;
        } 
        release(&p->lock);
      }
      if (foundMatchingPriority == 1){
        // si encontramos algun proceso en esta prioridad que sea RUNNABLE
        foundMatchingPriority = 0;
        curPrio--;
      }
    }

    if(found == 0) {
      // nothing to run; stop running on this core until an interrupt.
      intr_on();
      asm volatile("wfi");
    }
    

  }
}
```

En el scheduler optamos por un doble ciclo for con variable **curPrio** (la cual esta definida global mente para se accesible por la función allocproc) que va pasando por todas las prioridades. Si un proceso tiene las prioridad, usamos la variable **foundMatchingPriority** la cual funciona como marcador para saber que debemos ejecutar los procesos en esta prioridad pero luego no debemos avanzar a la siguiente prioridad, sino que debemos volver a ver si es que hay procesos con dicha prioridad. Esto es porque aunque un proceso se haya ejecutado, no significa que se haya terminado.

También tenemos un pequeño control de seguridad (que puede ser redundante), el cual  puede ser redundante, ya que no se puede alocar un procesos con un aprioridad diferrente de 0 caso en el cual la funcion allocproc se encarga de resetear **curPrio** a 0 y de asignar **found matching priority**  a 1 ya que sabemos que hay un proceso que se puede ejecutar con prioridad 0.

## Programa de prueba
el programa de prueba tiene el nombre de **priority_test.c** este programa crea variaos proceosos hijos los cuales tienen un **sleep**.

Aquí encontramos el mayor desafío, ya que es difícil diferenciar si es que una proceso esta durmiendo o esta ejecutando parte de su código, de esta forma es difícil saber cual seria exactamente el output que deberían tener los print.  Para esto agregamos pirnts tanto en la función de awake con sleep, con el objetivo de poder visualizar mejor el output, sin embargo puede ser un poco enredado de principio y se pueden quitar comentando estos print.
Sin embargo sigue sin ser claro para nosotros si el comportamiento de nuestro scheduler es exactamente el esperado.
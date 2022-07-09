TP3: Multitarea con desalojo
============================

sys_yield
---------
### Leer y estudiar el código del programa user/yield.c. Cambiar la función i386_init() para lanzar tres instancias de dicho programa, y mostrar y explicar la salida de make qemu-nox.

Lo que hace el código de user/yield es una iteración y en cada una se va llamando a la syscall yield, la cual invoca al Round Robin que implementamos en **sched_yield**.
Una vez creados, se ejecutan los programas de usuario y se puede ver cómo las distintas salidas se van "intercalando".

Round Robin da siempre el mismo tiempo de ejecución a los procesos sin importar como estos sean. En este caso, al ser los 3 iguales siempre llegan a ejecutar la misma cantidad de código en cada iteración. Vemos también que al hacer correr el programa varias veces los procesos terminan la última iteración en distinto orden, esto se debe a que el scheduler no utiliza prioridades para elegir qué proceso corre.


```
[00000000] new env 00001000
[00000000] new env 00001001
[00000000] new env 00001002
Hello, I am environment 00001000.
Hello, I am environment 00001001.
Hello, I am environment 00001002.
Back in environment 00001000, iteration 0.
Back in environment 00001001, iteration 0.
Back in environment 00001002, iteration 0.
Back in environment 00001000, iteration 1.
Back in environment 00001001, iteration 1.
Back in environment 00001002, iteration 1.
Back in environment 00001000, iteration 2.
Back in environment 00001001, iteration 2.
Back in environment 00001002, iteration 2.
Back in environment 00001000, iteration 3.
Back in environment 00001001, iteration 3.
Back in environment 00001002, iteration 3.
Back in environment 00001000, iteration 4.
All done in environment 00001000.
[00001000] exiting gracefully
[00001000] free env 00001000
Back in environment 00001001, iteration 4.
All done in environment 00001001.
[00001001] exiting gracefully
[00001001] free env 00001001
Back in environment 00001002, iteration 4.
All done in environment 00001002.
[00001002] exiting gracefully
[00001002] free env 00001002
No runnable environments in the system!
```

```
[00000000] new env 00001000
[00000000] new env 00001001
[00000000] new env 00001002
Hello, I am environment 00001000.
Hello, I am environment 00001001.
Hello, I am environment 00001002.
Back in environment 00001000, iteration 0.
Back in environment 00001001, iteration 0.
Back in environment 00001002, iteration 0.
Back in environment 00001000, iteration 1.
Back in environment 00001001, iteration 1.
Back in environment 00001002, iteration 1.
Back in environment 00001000, iteration 2.
Back in environment 00001001, iteration 2.
Back in environment 00001002, iteration 2.
Back in environment 00001000, iteration 3.
Back in environment 00001001, iteration 3.
Back in environment 00001002, iteration 3.
Back in environment 00001000, iteration 4.
All done in environment 00001000.
[00001000] exiting gracefully
[00001000] free env 00001000
Back in environment 00001002, iteration 4.
All done in environment 00001002.
[00001002] exiting gracefully
[00001002] free env 00001002
Back in environment 00001001, iteration 4.
All done in environment 00001001.
[00001001] exiting gracefully
[00001001] free env 00001001
No runnable environments in the system!
```

---

dumbfork
--------
### 1) Si una página no es modificable en el padre ¿lo es en el hijo? En otras palabras: ¿se preserva, en el hijo, el flag de solo-lectura en las páginas copiadas?

No, no se preserva. En la implementación de dumbfork se llama a duppage, y en ella siempre se le pasan los mismos permisos PTE_P|PTE_U|PTE_W. Luego a partir de ahi no se realiza ningun chequeo de permisos con lo cual siempre va a ser modificable por el hijo y el flag de sólo lectura no se preservará.


### 2) Mostrar, con código en espacio de usuario, cómo podría dumbfork() verificar si una dirección en el padre es de solo lectura, de tal manera que pudiera pasar como tercer parámetro a duppage() un booleano llamado readonly que indicase si la página es modificable o no.

Con uvpt podemos chequear los permisos que tiene la page, en este caso chequeamos contra el flag de escritura.

```
envid_t dumbfork(void) {
    // ...
    for (addr = UTEXT; addr < end; addr += PGSIZE) {
        bool readonly = false;
        
        if ((uvpd[PDX(addr)] & PTE_P) == PTE_P){
            if ((uvpt[PGNUM(addr)] & PTE_W) != PTE_W){
                readonly = true;
            }
        }
        
        duppage(envid, addr, readonly);
    }
    // ...
```

### 3)Supongamos que se desea actualizar el código de duppage() para tener en cuenta el argumento readonly: si este es verdadero, la página copiada no debe ser modificable en el hijo. Es fácil hacerlo realizando una última llamada a sys_page_map() para eliminar el flag PTE_W en el hijo, cuando corresponda. Esta versión del código, no obstante, incrementa las llamadas al sistema que realiza duppage() de tres, a cuatro. Se pide mostrar una versión en el que se implemente la misma funcionalidad readonly, pero sin usar en ningún caso más de tres llamadas al sistema. 

```
void duppage(envid_t dstenv, void *addr, bool readonly) {
    // Código original (simplificado): tres llamadas al sistema.
    int perm = PTE_P | PTE_U | PTE_W;
    if(readonly){
        perm &= ~PTE_W;
    }
    sys_page_alloc(dstenv, addr, perm);
    sys_page_map(dstenv, addr, 0, UTEMP, perm);
    memmove(UTEMP, addr, PGSIZE);
    sys_page_unmap(0, UTEMP);
}
```
---

ipc_recv
--------

### Un proceso podría intentar enviar el valor númerico -E_INVAL vía ipc_send(). ¿Cómo es posible distinguir si es un error, o no?

En ipc_recv si se genera un error el src (si no es nulo) se setea a cero y se devuelve el error, entonces:

```
envid_t src = -1;
int r = ipc_recv(&src, 0, NULL);

if (r < 0)
  if (src == 0)
    puts("Hubo error.");
  else
    puts("Valor negativo correcto.")
```

sys_ipc_try_send
----------------

### 1) qué cambios se necesitan en struct Env para la implementación (campos nuevos, y su tipo; campos cambiados, o eliminados, si los hay)

Opción 1:
```
struct Env {
	// ...
    envid_t env_id_waiting; // envid de un proceso bloqueado, que esta esperando para mandarme un mensaje	
};
```

Opción 2:
```
struct Env {
	// ...
    struct Env *env_waiting; // env bloqueado, que esta esperando para mandarme un mensaje	
}
```
### 2) qué asignaciones de campos se harían en sys_ipc_send()


Opción 1:

```
sys_ipc_send(envid_t envid, uint32_t value, void *srcva, unsigned perm){
    // ...

    if (!(env->env_ipc_recving)) {
		env->env_id_waiting = curenv->env_id;
	    curenv->env_status = ENV_NOT_RUNNABLE;
	}

    // ...
}
```

La opción 2 es similar pero con env->env_waiting = curenv;

### 3) qué código se añadiría en sys_ipc_recv()

```
sys_ipc_recv(void *dstva){
	// ...

	curenv->env_ipc_recving = true;
    if(curenv->env_id_waiting){
        sys_env_set_status(curenv->env_id_waiting, ENV_RUNNABLE);
        curenv->env_id_waiting = NULL;
    }
	curenv->env_status = ENV_NOT_RUNNABLE;
	curenv->env_tf.tf_regs.reg_eax = 0;
	return 0;
}
```

La opción 2 es similar pero con sys_env_set_status(curenv->env_waiting.env_id, ENV_RUNNABLE);

### 4) ¿existe posibilidad de deadlock?

Si, si el scheduler decide poner a ejecutar al proceso (A en el ejemplo) que esta esperando enviar un mensaje justo después de que desde sys_ipc_recv se le cambie el status, este proceso le va a cambiar el estado al proceso receptor (B) a ENV_RUNNABLE. Luego, cuando vuelva la ejecución a sys_ipc_recv de B, este va a cambiar a ENV_NOT_RUNNABLE y va a quedar bloqueado por siempre (ya que el código que lo desbloquea ya se ejecuto).

Una posible implementación si se podría usar locks sería:

```
sys_ipc_send(envid_t envid, uint32_t value, void *srcva, unsigned perm){
    // ...

    if (!(env->env_ipc_recving)) {
		env->env_id_waiting = curenv->env_id;
	    curenv->env_status = ENV_NOT_RUNNABLE;
	}
    lock(l1);
    // ...                          // si hay un if que devuelve un error, hace el unlock antes
	env->env_ipc_value = value;
    env->env_ipc_from = curenv->env_id;
	env->env_ipc_recving = false;
	env->env_status = ENV_RUNNABLE;
    unlock(l1);
    env->env_tf.tf_regs.reg_eax = 0;
    // ...

}
```

```
sys_ipc_recv(void *dstva){
	// ...

	curenv->env_ipc_recving = true;
    lock(l1);
    if(curenv->env_id_waiting){
        sys_env_set_status(curenv->env_id_waiting, ENV_RUNNABLE);
        curenv->env_id_waiting = NULL;
    }
	curenv->env_status = ENV_NOT_RUNNABLE;
    unlock(l1);
	curenv->env_tf.tf_regs.reg_eax = 0;
	return 0;

	// ...
}
```
Otra posibilidad de deadlock puede ser cuando el proceso A le quiere enviar algo a B y además B le quiere mandar un mensaje a A. Ambos quedarían en estado ENV_NOT_RUNNABLE (xq el otro proceso no espera un mensaje) y nunca se desbloquearían. Una posible solución a esto, sería que antes de que un proceso se vaya a bloquear se fije si hay algún proceso que esta esperando para mandarle algo, en este caso no se bloquearía.

### 5) ¿funciona que varios procesos (A₁, A₂, …) puedan enviar a B, y quedar cada uno bloqueado mientras B no consuma su mensaje? ¿en qué orden despertarían?

Para la implementación actual no funcionaría pero se podría implementar el atributo env_id_waiting como un array de env_id's (una pila por ejemplo) y cada vez que se llame a sys_ipc_recv se desbloquea alguno de los procesos del array y se lo elimina del mismo.

El orden en el que se despertarán puede ser del primero al último o viceversa, según como se quiera implementar.





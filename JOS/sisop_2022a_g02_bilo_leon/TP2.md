TP2: Procesos de usuario
========================

env_alloc
---------

### ¿Qué identificadores se asignan a los primeros 5 procesos creados? (Usar base hexadecimal)

La creacion del id es único para cada proceso y se calcula en base al siguiente calculo:

```
generation = (e->env_id + (1 << ENVGENSHIFT)) & ~(NENV - 1);
e->env_id = generation | (e - envs);
```

Donde 
  - 1 << ENVGENSHIFT = 4096|10 = 0x1000
  - ~(NENV - 1) = 4294966270|10 = 0XFFFFFBFE 

Estos valores siempre serán constantes. Para los primeros 5 procesos el e->env_id será cero porque aún no se necesitan reciclar procesos (5 < 1024 (NENV)). La suma de 0 y 0x1000 es de 0x1000 y haciendo un and con 0x0XFFFFFBFE, dará 0x1000.

El int32 generation será entonces: 0x1000 para estos cinco procesos.

El primero offset es 0 ya que el primer env equivale a la dirección del envs que representa todo el arreglo. Por cada nuevo proceso se suma 1 con lo cual:

  1. env_id1 = 0x1000 
  2. env_id2 = 0x1001
  2. env_id3 = 0x1002
  2. env_id4 = 0x1003
  2. env_id5 = 0x1004


### Supongamos que al arrancar el kernel se lanzan NENV procesos a ejecución. A continuación, se destruye el proceso asociado a envs[630] y se lanza un proceso que cada segundo, muere y se vuelve a lanzar (se destruye, y se vuelve a crear). ¿Qué identificadores tendrán esos procesos en las primeras cinco ejecuciones?

Como llegamos a un punto en donde todo el arreglo de envs ya tenía asociado un proceso, cualquier nuevo proceso que se genere usará el id del environment previo en el cálculo del generation. Es decir, si se destruye el proceso asociado a envs[630], el nuevo proceso que ocupará su lugar utilizará el id del environment destruido para generar el nuevo id. Y lo mismo pasará con los procesos subsiguientes. Si se destruyen y ocurren cada un segundo, muy probablemente ocupen siempre el mismo env y podemos calcular sus ids. Como se verá, el generation en este caso es variable y el offset constante (630, 276 en hexa).

  1. generation = 0x1000
     env[630]->env_id = 0x1000 | 0x276 = 0x1276

  2. env_id = ((0x1276 + 0x1000) & 0XFFFFFBFE) | 0x276 = 0x2276
  
  3. env_id = ((0x2276 + 0x1000) & 0XFFFFFBFE) | 0x276 = 0x3276

  4. env_id = ((0x3276 + 0x1000) & 0XFFFFFBFE) | 0x276 = 0x4276
  
  5. env_id = ((0x4276 + 0x1000) & 0XFFFFFBFE) | 0x276 = 0x5276

env_pop_tf
----------


### Dada la secuencia de instrucciones assembly en la función, describir qué contiene durante su ejecución:
###      - el tope de la pila justo antes popal
###      - el tope de la pila justo antes iret
###      - el tercer elemento de la pila justo antes de iret

El tope de la pila justo antes de popal contendrá el valor de la direccion a la que apunta tf, recibido por parametro (movl %0, %%esp). 

Se hace un pop (se desapilan) primero de los registros de proposito general y de los registros %es y %ds. Se saltean los registros de errorno y err_code (addl 0x8, %%esp) y la %esp llega al registro de eip. Por lo que el tope de la pila justo antes de iret contendrá el tf->eip.

El tercer elemento de la pila entonces contendrá la los flags del trapframe (tf->eflags).

### En la documentación de iret en [IA32-2A] se dice:

###     If the return is to another privilege level, the IRET instruction also pops the stack pointer and SS from the stack, before resuming program execution.

### ¿Cómo determina la CPU (en x86) si hay un cambio de ring (nivel de privilegio)? Ayuda: Responder antes en qué lugar exacto guarda x86 el nivel de privilegio actual. ¿Cuántos bits almacenan ese privilegio?

La CPU en x86 compara el CPL (Current Privilege Level) guardado en dos bits del CS (Current Code segment register). Si el CPL es distinto al DPL (Descriptor Privilege Level), entonces hay un cambio de privilegios.


gdb_hello
---------

### Poner un breakpoint en env_pop_tf() y continuar la ejecución hasta allí.

```
(gdb) b env_pop_tf
Punto de interrupción 1 at 0xf0102f17: file kern/env.c, line 477.
(gdb) c
Continuando.
Se asume que la arquitectura objetivo es i386
=> 0xf0102f17 <env_pop_tf>:	endbr32 

Breakpoint 1, env_pop_tf (tf=0xf0203000) at kern/env.c:477
477	{
```

### En QEMU, entrar en modo monitor (Ctrl-a c), y mostrar las cinco primeras líneas del comando info registers.

```
(qemu) info registers
EAX=003bc000 EBX=00010094 ECX=f03bc000 EDX=0000020c
ESI=00010094 EDI=00000000 EBP=f010dfd8 ESP=f010dfbc
EIP=f0102f17 EFL=00000092 [--S-A--] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
```

### De vuelta a GDB, imprimir el valor del argumento tf:

```
(gdb) p tf
$1 = (struct Trapframe *) 0xf0203000
```

### Imprimir, con x/Nx tf tantos enteros como haya en el struct Trapframe donde N = sizeof(Trapframe) / sizeof(int).

```
(gdb) print sizeof(struct Trapframe) / sizeof(int)
$2 = 17
(gdb) x/17x tf
0xf01c0000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c0030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c0040:	0x00000023
```

### Avanzar hasta justo después del movl ...,%esp, usando si M para ejecutar tantas instrucciones como sea necesario en un solo paso.

```
(gdb) disas
Dump of assembler code for function env_pop_tf:
=> 0xf0102fb1 <+0>:	endbr32 
   0xf0102fb5 <+4>:	push   %ebp
   0xf0102fb6 <+5>:	mov    %esp,%ebp
   0xf0102fb8 <+7>:	sub    $0xc,%esp
   0xf0102fbb <+10>:	mov    0x8(%ebp),%esp
   0xf0102fbe <+13>:	popa   
   0xf0102fbf <+14>:	pop    %es
   0xf0102fc0 <+15>:	pop    %ds
   0xf0102fc1 <+16>:	add    $0x8,%esp
   0xf0102fc4 <+19>:	iret   
   0xf0102f2b <+20>:	push   $0xf010547f
   0xf0102f30 <+25>:	push   $0x1e7
   0xf0102f35 <+30>:	push   $0xf010541e
   0xf0102fd4 <+35>:	call   0xf01000ad <_panic>
End of assembler dump.
(gdb) si 5
=> 0xf0102f24 <env_pop_tf+13>:	popa   
0xf0102f24 in env_pop_tf (tf=0x0) at kern/env.c:478
478		asm volatile("\tmovl %0,%%esp\n"
```

### Comprobar, con x/Nx $sp que los contenidos son los mismos que tf (donde N es el tamaño de tf).

Igual que tf por lo que dijimos anteriormente (justo antes de popal, el sp apunta a la misma dirección que el tf).

```
(gdb) x/17x $sp
0xf01c0000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c0030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c0040:	0x00000023
```

### Describir cada uno de los valores. Para los valores no nulos, se debe indicar dónde se configuró inicialmente el valor, y qué representa.

```
reg_edi       0x00000000        Registro de proposito general
reg_esi       0x00000000        Registro de proposito general
reg_ebp       0x00000000        Registro de proposito general
reg_oesp      0x00000000        Registro de proposito general
reg_ebx       0x00000000        Registro de proposito general
reg_edx       0x00000000        Registro de proposito general
reg_ecx       0x00000000        Registro de proposito general
reg_eax       0x00000000        Registro de proposito general
tf_es         0x00000023        Puntero al Data Segment                 env_alloc
tf_ds         0x00000023        Puntero al Data Segment                 env_alloc
tf_trapno     0x00000000        Trap Number                              
tf_err        0x00000000        Codigo de error                         
tf_eip        0x00800020        Instruction Pointer                     load_icode
tf_cs         0x0000001b        Puntero al Code Segment                 env_alloc
tf_eflags     0x00000000        Flags del tf                  
tf_esp        0xeebfe000        Puntero al stack                        env_alloc
tf_ss         0x00000023        Stack segment                           env_alloc
```

### Continuar hasta la instrucción iret, sin llegar a ejecutarla. Mostrar en este punto, de nuevo, las cinco primeras líneas de info registers en el monitor de QEMU. Explicar los cambios producidos.

Vemos que los registros de proposito general se han limpiado y el DPL del datasegment  paso de modo kernel a modo usuario.

```
(qemu) info registers
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=00000000 ESP=f0203030
EIP=f0102f2a EFL=00000096 [--S-AP-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
```


### Ejecutar la instrucción iret. En ese momento se ha realizado el cambio de contexto y los símbolos del kernel ya no son válidos.

GDB

```
(gdb) si 1
=> 0x800020:	cmp    $0xeebfe000,%esp
0x00800020 in ?? ()
(gdb) p $eip
$3 = (void (*)()) 0x800020
(gdb) add-symbol-file obj/user/hello 0x800020
add symbol table from file "obj/user/hello" at
	.text_addr = 0x800020
(y or n) y
Reading symbols from obj/user/hello...
(gdb) p $eip
$4 = (void (*)()) 0x800020 <_start>
```
QEMU

```
(qemu) info registers
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=00000000 ESP=eebfe000
EIP=00800020 EFL=00000002 [-------] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
```

Ahora todos los registros de segmento pasaron a user mode esto es porque el CPL indicó un cambio de contexto pasando de 0 a 3. Y se muestran los mismos valores que el tf.

### Poner un breakpoint temporal (tbreak, se aplica una sola vez) en la función syscall() y explicar qué ocurre justo tras ejecutar la instrucción int $0x30. Usar, de ser necesario, el monitor de QEMU

```
(gdb) tbreak syscall
Punto de interrupción temporal 2 at 0x800a2d: syscall. (2 locations)
(gdb) c
Continuando.
=> 0x800a2d <syscall>:	push   %ebp

Temporary breakpoint 2, syscall (num=num@entry=0, check=check@entry=0, a1=4005551752, a1@entry=0, a2=13, a3=0, a4=0, a5=0)
    at lib/syscall.c:8
8	{

(gdb) si
=> 0x800a4a <syscall+29>:	int    $0x30
0x00800a4a	23		asm volatile("int %1\n"
(gdb) si
Se asume que la arquitectura objetivo es i8086
[f000:e05b]    0xfe05b:	cmpw   $0xffc8,%cs:(%esi)
0x0000e05b in ?? ()
```
Previa ejecución del tbreak:
```
(qemu) info registers
EAX=00000000 EBX=00000000 ECX=eebfde88 EDX=00000000
ESI=00000000 EDI=00000000 EBP=eebfde60 ESP=eebfde44
EIP=00800a2d EFL=00000092 [--S-A--] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
```

Luego de la ejecución de int $0x30:
```
(qemu) info registers
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000663
ESI=00000000 EDI=00000000 EBP=00000000 ESP=00000000
EIP=0000e05b EFL=00000002 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0000 00000000 0000ffff 00009300
CS =f000 000f0000 0000ffff 00009b00
```

Se puede ver que pasamos de user mode a kernel mode, esto se debe a que el comando int $0x30 genera una interrupción de software.
Lo que paso, es que se corto la ejecución ya que todavía nuestra JOS no tiene implementado las funciones que manejan las interrupciones y syscalls.

Luego de completar la parte 4 del TP, podemos ver que la ejecución de int $0x30 sigue normalmente:

```
(gdb) si
=> 0x800a5a <syscall+29>:	int    $0x30
0x00800a5a	23		asm volatile("int %1\n"
(gdb) si
=> 0xf01038bc <trap_48+2>:	push   $0x30
0xf01038bc in trap_48 () at kern/trapentry.S:67
67	TRAPHANDLER_NOEC(trap_48, T_SYSCALL)
```

kern_idt
--------

### Leer user/softint.c y ejecutarlo con make run-softint-nox. ¿Qué interrupción trata de generar? ¿Qué interrupción se genera? Si son diferentes a la que invoca el programa… ¿cuál es el mecanismo por el que ocurrió esto, y por qué motivos? ¿Qué modificarían en JOS para cambiar este comportamiento?

```
TRAP frame at 0xf01c0000
  edi  0x00000000
  esi  0x00000000
  ebp  0xeebfdff0
  oesp 0xefffffdc
  ebx  0x00000000
  edx  0x00000000
  ecx  0x00000000
  eax  0xeec00000
  es   0x----0023
  ds   0x----0023
  trap 0x0000000d General Protection
  err  0x00000072
  eip  0x00800037
  cs   0x----001b
  flag 0x00000082
  esp  0xeebfdfd4
  ss   0x----0023
[00001000] free env 00001000
Destroyed the only environment - nothing more to do!
Welcome to the JOS kernel monitor!
Type 'help' for a list of commands.
K> QEMU: Terminated

```


El programa esta tratando de generar un Page fault pero JOS genera una General protection fault, esto es porque cuando se inicializa el handler de la trap de PGFLT a SETGATE le estamos pasando el ultimo parametro en 0 que representa el nivel de privilegio que se debe tener para ejecutarla. Para que se pueda ejecutar en user mode deberiamos cambiar la inicializacion de la trap de la siguiente manera:

de: 
```
SETGATE(idt[T_PGFLT], 0, GD_KT, trap_14, 0); 
```
a:
```
SETGATE(idt[T_PGFLT], 0, GD_KT, trap_14, 3); 
```



user_evilhello
--------------

### ¿En qué se diferencia el código de la versión en evilhello.c mostrada arriba?
    
En evilhello.c original se llama a sys_cputs con un puntero a char. Mientras que en el segundo caso, primero se hace una desreferencia al puntero a char y se llama a sys_cputs con esa desreferencia.

### ¿En qué cambia el comportamiento durante la ejecución? ¿Por qué? ¿Cuál es el mecanismo? 

En evilhello.c original no se genera ninguna interrupción ya que se le esta pasando un puntero a la dirección del kernel y como no se esta chequeando aún si la memoria a la que se quiere acceder puede ser accedida por el usuario, trata de imprimir el entrypoint del kernel (f�r).

En el programa modificado, se desreferencia el puntero a la dirección del kernel. Aunque no se haya chequeado si la memoria puede ser accedida por el usuario, salta antes un PageFault porque se esta tratando de acceder directo a memoria del kernel.

### Listar las direcciones de memoria que se acceden en ambos casos, y en qué ring se realizan. ¿Es esto un problema? ¿Por qué?

En ambos casos se esta tratando de accender a direcciones de memoria del kernel, las cuales están en el ring 0. El usuario que es el que esta intentando entrar, pertenece al ring 3. Esto SI es un problema, ya que el usuario esta intentando ingresar a direcciones de las cuales no tiene permisos.


Direcciones de memoria que se acceden en ambos casos:

evilhello.c (original):
```
[00000000] new env 00001000
Incoming TRAP frame at 0xefffffbc
Incoming TRAP frame at 0xefffffbc
f�rIncoming TRAP frame at 0xefffffbc
[00001000] exiting gracefully
[00001000] free env 00001000
```

evilhello.c (modificado):
```
[00000000] new env 00001000
Incoming TRAP frame at 0xefffffbc
Incoming TRAP frame at 0xefffffbc
[00001000] user fault va f010000c ip 0080003d
TRAP frame at 0xf01c0000
  edi  0x00000000
  esi  0x00000000
  ebp  0xeebfdfd0
  oesp 0xefffffdc
  ebx  0x00000000
  edx  0x00000000
  ecx  0x00000000
  eax  0xeec000c0
  es   0x----0023
  ds   0x----0023
  trap 0x0000000e Page Fault
  cr2  0xf010000c
  err  0x00000005 [user, read, protection]
  eip  0x0080003d
  cs   0x----001b
  flag 0x00000082
  esp  0xeebfdfb0
  ss   0x----0023
[00001000] free env 00001000
```
...


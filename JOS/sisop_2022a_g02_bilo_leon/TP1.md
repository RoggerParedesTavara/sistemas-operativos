TP1: Memoria virtual en JOS
===========================

boot_alloc_pos
--------------

### a) Un cálculo manual de la primera dirección de memoria que devolverá boot_alloc() tras el arranque. Se puede calcular a partir del binario compilado (obj/kern/kernel), usando los comandos readelf y/o nm y operaciones matemáticas.

```
agustina@agustina-Ubuntu: readelf -a kernel | grep end
106: f0117980     0 NOTYPE  GLOBAL DEFAULT    6 end
```

```
agustina@agustina-Ubuntu: nm kernel | grep end
f0117980 B end
```

El valor de end en hexadecimal es f0117980 y pasado a decimal es 4027677056.
Dentro del boot_alloc se le aplica un redondeo al múltiplo de PGSIZE mas cercan, o sea:

ROUNDUP(4027677056, 4096) == 4027678720

El valor de este redondeo es lo que devolverá boot_alloc en su primera ejecución.

--- 

### b) Una sesión de GDB en la que, poniendo un breakpoint en la función boot_alloc(), se muestre el valor devuelto en esa primera llamada, usando el comando GDB finish.

```
agustina@agustina-Ubuntu: make gdb
(gdb) break boot_alloc
Breakpoint 1 at 0xf0100a5f: file kern/pmap.c, line 88.
(gdb) continue
Continuing.
The target architecture is set to "i386".
=> 0xf0100a5f <boot_alloc>:	push   %ebp

Breakpoint 1, boot_alloc (n=4096) at kern/pmap.c:88
88	{
(gdb) print (char *) &end
$1 = 0xf0117980 ""
(gdb) watch nextfree
Hardware watchpoint 3: nextfree
(gdb) continue
Continuing.
=> 0xf0100ab6 <boot_alloc+87>:	jmp    0xf0100a6f <boot_alloc+16>

Hardware watchpoint 3: nextfree

Old value = 0x0
New value = 0xf0118000 ""
0xf0100ab6 in boot_alloc (n=4096) at kern/pmap.c:99
99			nextfree = ROUNDUP((char *) end, PGSIZE);
(gdb) continue
Continuing.
=> 0xf0100a9e <boot_alloc+63>:	mov    %ecx,%eax

Hardware watchpoint 3: nextfree

Old value = 0xf0118000 ""
New value = 0xf0119000 ""
boot_alloc (n=4027682816) at kern/pmap.c:117
117	}
```

End, como habiamos visto, tiene el valor de 0xf0117980.
También vemos que nextfree empieza en 0x0 y luego pasa a 0xf0118000.
Finalmente, se puede observar que se avanza una página y queda en 0xf0119000.


---

page_alloc
----------


---


map_region_large
----------------
### Responder las siguientes dos preguntas, específicamente en el contexto de JOS:

### a) ¿Cuánta memoria se ahorró de este modo? (en KiB)

Con la implementación de páginas grandes se ahorran 4KiB por cada una que se usa.. Ya que al eliminar una página grande eliminamos la pgtable correspondiente la cual cuenta con 1024 entradas de 32 bits cada una. 1024 * 32 = 32768 bits = 4KiB.

En este caso usamos boot_map_region tres veces:

En las primeras dos no se ahorran porque: en el mapeo de pages no se utiliza large pages porque UPAGES esta alineada pero la direccion fisica no (0x001aa000) y en el mapeo del stack la cantidad de memoria mapeada es inferior a 4Mb (32Kb).

Ya en la tercera llamada si se ahorra en page table porque se mapean 256Mb de memoria y las direcciones ambas fisica y virtual estan alineadas (0x00 y 0xF00000).

Si se mapean 256Mb en large pages de 4Mb cada una, se mapean 64 paginas. Si por cada pagina se ahorran 1024 page tables entonces => 64 paginas * 4Kb = 256Kb.

### b) ¿Es una cantidad fija, o depende de la memoria física de la computadora?

Se trata de una cantidad fija que sólo depende del contexto de la arquitectura. En el caso de JOS, con una arquitectura de 32 bits, se ahorran siempre 256KiB.
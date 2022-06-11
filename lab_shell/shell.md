# Lab: shell

### Búsqueda en $PATH

#### ¿Cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?
La syscall execve(2) ejecuta el programa que recibe, reemplazando/sobreescribiendo el espacio de memoria del proceso que la llamo con un nuevo stack, heap y data segments.
En cambio, la familia de exec(3) llama a la syscall execve(2). Esta familia proporciona distintas formas de implementarlas, facilitando la utilización de la syscall antes mencionada.


#### ¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?
Si, puede fallar cuando ocurre la llamada a execve(2), por los mismos errores de esta, devolviendo -1. Al fallar no se hace el reemplazo del espacio de memoria del proceso.
La shell va a ejecutar el código que avisa al usuario del error y luego espera a la ejecución de un nuevo comando.


---

### Comandos built-in

#### ¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false)

`cd` debe ser implementado como built-in, ya que se debe cambiar el directorio de la shell y no de un hijo (este ultimo caso pasaría si no fuera built-in).

`pwd` se puede no implementarlo como built-in ya que solo muestra el lugar donde esta "parada" la shell en ese momento. Pero, al hacerlo built-in es mucho más eficiente, ya que se hace de forma directa evitando la implementación de más instrucciones (fork y exec).

---

### Variables de entorno adicionales

#### Variables de entorno temporales.

#### ¿Por qué es necesario hacerlo luego de la llamada a fork(2)?

Justamante, al ser variables temporales solo deben existir durante la ejecución del programa y no en la shell. O sea, se deben setear luego del fork y antes de la ejecución, y así estaran definidas en el proceso del programa a ejecutar.


#### En algunos de los wrappers de la familia de funciones de exec(3) (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar setenv(3) por cada una de las variables, se guardan en un array y se lo coloca en el tercer argumento de una de las funciones de exec(3). 

#### ¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.

No, el comportamiento no es el mismo. En este caso estaríamos solo seteando las variables de entorno que se reciben en el tercer argumento.

#### Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.

Se deberian pasar como 3er argumento TODAS las variables del entorno (incluidas las temporales y los de la shell). Para esto se puede utilizar la variable global `extern char **environ` (se le deberían agregar las variables de entorno temporales).


---

### Procesos en segundo plano

#### Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano.

El mecanismo utilizado consiste en que el proceso principal espera a la ejecución de los procesos en segundo plano para no dejarlos huerfanos, pero de una forma que **NO** bloquea a la shell gracias al flag  `WNOHANG`. Además, en el segundo argumento del waitpid() se va a guardar el estado que devuelve el proceso en segundo plano.

---

### Flujo estándar

#### Investigar el significado de 2>&1, explicar cómo funciona su forma general y mostrar qué sucede con la salida de cat out.txt en el ejemplo. Luego repetirlo invertiendo el orden de las redirecciones. ¿Cambió algo?

`2>&1`: Este comando significa que se debe hacer una redirección (>), en este caso se debe redirigir la salida del file descriptor 2 (stderr) hacia el file descriptor &1 (stdout).
`>out.txt 2>&1`: En el ejemplo, primero se redirige la salida del fd 1 hacia out.txt (todo lo que se mande al fd 1 se va a guardar en el archivo out.txt). Luego al hacer ls de un archivo/directorio que no existe va a arrojar un error en (stderr) pero este fue redirigido hacia &1, entonces se va a guardar en out.txt.

`Ejemplo 1:`
```
$ ls -C /home /noexiste >out.txt 2>&1
$ cat out.txt
ls: cannot access '/noexiste': No such file or directory
/home:
lucas
```

`2>&1 >out1.txt`: Al invertir el orden, lo que sucede es que se va a imprimir el error por la salida estandar (por la redirección 2>&1), ya que hasta ese momento no se hizo la redirección >out.txt. Pero, como al final si se hace esa redirección, la salida de ls si se va a guardar en el out.txt y no se va a imprimir por la salida estandar.

`Ejemplo 2:`
```
$ ls -C /home /noexiste 2>&1 >out1.txt
ls: cannot access '/noexiste': No such file or directory
lucas@lucas-VirtualBox:~/Desktop/mylabs/shell$ cat out3.txt
/home:
lucas
```

---

### Tuberías simples (pipes)

#### Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe ¿Cambia en algo? ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con la implementación del este lab. 

La shell(bash) reporta el exit code del ultimo comando ejecutado en las tuberías simples (pipes). Si hay un fallo en el primer comando del pipe no importa, siempre devolverá el exit code del segundo.

`Ejemplo 1 (sin errores):`
```
lucas@lucas-VirtualBox:~/Desktop/mylabs/shell$ ls -l | grep D
-rw-rw-r-- 1 lucas lucas    334 Apr 13 10:47 README.md
lucas@lucas-VirtualBox:~/Desktop/mylabs/shell$ echo $?
0
```
`Ejemplo 2 (error en primer comando):`
```
lucas@lucas-VirtualBox:~/Desktop/mylabs/shell$ ls /noexiste | echo hola
hola
ls: cannot access '/noexiste': No such file or directory
lucas@lucas-VirtualBox:~/Desktop/mylabs/shell$ echo $?
0
```
`Ejemplo 3 (error en segundo comando):`
```
lucas@lucas-VirtualBox:~/Desktop/mylabs/shell$ ls -l | grepa d
Command 'grepa' not found, did you mean: ...
lucas@lucas-VirtualBox:~/Desktop/mylabs/shell$ echo $?
127
```

En la shell que yo construí el exit code del pipe siempre será 0, debido que al finalizar el fork hay un _exit(0). La diferencia con respecto a bash y mi *shell* es que en ningún momento se actualiza el status principal de la ejecución con los exit code de los hijos, sin importar si alguno lanza un error o no.
El único caso donde se puede devolver algo distinto de cero es si el fork falla.

`Ejemplo 1 (sin errores):`
```
 (/home/lucas/Desktop/mylabs/shell) 
$ ls -l | grep D
-rw-rw-r-- 1 lucas lucas    334 Apr 13 10:47 README.md
 (/home/lucas/Desktop/mylabs/shell) 
$ echo $?
0
```
`Ejemplo 2 (error en primer comando):`
```
 (/home/lucas/Desktop/mylabs/shell) 
$ ls /noexiste | echo hola
ls: cannot access '/noexiste': No such file or directory
hola
 (/home/lucas/Desktop/mylabs/shell) 
$ echo $?
0
```
`Ejemplo 3 (error en segundo comando):`
```
 (/home/lucas/Desktop/mylabs/shell) 
$ ls -l | grepa d
Error: No such file or directory
 (/home/lucas/Desktop/mylabs/shell) 
$ echo $?
0
```

---

### Pseudo-variables

#### Investigar al menos otras tres variables mágicas estándar, y describir su propósito. Incluir un ejemplo de su uso en bash (u otra terminal similar).

- `$$`: Devuelve el PID de la shell que se está ejecutando actualmente.
```
lucas@lucas-VirtualBox:~/Desktop$ echo $$
8718
```

- `$!`: Devuelve el PID del último proceso ejecutado en segundo plano
```
lucas@lucas-VirtualBox:~/Desktop/mylabs/shell$ evince README.md &
[1] 12631
lucas@lucas-VirtualBox:~/Desktop/mylabs/shell$ ls /home
lucas
lucas@lucas-VirtualBox:~/Desktop/mylabs/shell$ echo $!
12631
```

- `$_`: Devuelve al último argumento del comando anterior
```
lucas@lucas-VirtualBox:~/Desktop$ echo sisop lab2 shell
sisop lab2 shell
lucas@lucas-VirtualBox:~/Desktop$ echo $_
shell

lucas@lucas-VirtualBox:~/Desktop/mylabs$ ls -l
total 8
drwxrwxr-x 3 lucas lucas 4096 Apr 13 10:47 fork
drwxrwxr-x 3 lucas lucas 4096 Apr 19 23:05 shell
lucas@lucas-VirtualBox:~/Desktop/mylabs$ echo $_
-l
```

---



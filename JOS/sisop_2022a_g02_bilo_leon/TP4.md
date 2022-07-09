TP4: Sistema de archivos e intérprete de comandos
=================================================

caché de bloques
----------------

### ¿Qué es super->s_nblocks?

Este valor es el número total de bloques en el disco.

### ¿Dónde y cómo se configura este bloque especial?

Este bloque especial se va a encontrar siempre en el bloque 1 del disco. Dicho bloque se configura en la función **opendisk**, la cual se encuentra en fs/fsformat.c

```
void
opendisk(const char *name)
{
	...

	alloc(BLKSIZE);
	super = alloc(BLKSIZE);
	super->s_magic = FS_MAGIC;
	super->s_nblocks = nblocks;
	super->s_root.f_type = FTYPE_DIR;
	
    ...
}
```
...


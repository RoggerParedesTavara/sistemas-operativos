// evil hello world -- kernel pointer passed to kernel
// kernel should destroy user environment in response

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	// try to print the kernel entry point as a string!  mua ha ha!
	sys_cputs((char*)0xf010000c, 100);

	// char *entry = (char *) 0xf010000c;
    // char first = *entry;
    // sys_cputs(&first, 1);
}

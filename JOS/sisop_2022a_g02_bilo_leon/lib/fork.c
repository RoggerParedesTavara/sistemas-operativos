// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW 0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.

	if (!((uvpt[PGNUM(addr)] & PTE_COW)) || ((err & FEC_WR) == 0) ||
	    ((err & FEC_PR) == 0)) {
		panic("[pgfault]: error");
	}
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	if ((r = sys_page_alloc(0, PFTEMP, PTE_P | PTE_U | PTE_W)) < 0)
		panic("[sys_page_alloc]: %e", r);

	memmove(PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);

	if ((r = sys_page_map(
	             0, PFTEMP, 0, ROUNDDOWN(addr, PGSIZE), PTE_P | PTE_U | PTE_W)) <
	    0)
		panic("[sys_page_map]: %e", r);

	if ((r = sys_page_unmap(0, PFTEMP)) < 0)
		panic("[sys_page_unmap]: %e", r);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	void *va = (void *) (pn * PGSIZE);
	int perm = uvpt[pn];

	// LAB 4: Your code here.

	if (perm & PTE_SHARE)
		return sys_page_map(thisenv->env_id, va, envid, va, perm);


	if (perm & PTE_W)
		perm = (perm & ~PTE_W) | PTE_COW;

	if ((r = sys_page_map(thisenv->env_id, va, envid, va, perm)) < 0)
		return r;

	if (perm & PTE_COW) {
		if ((r = sys_page_map(
		             thisenv->env_id, va, thisenv->env_id, va, perm)) < 0)
			return r;
	}

	return 0;
}

static void
dup_or_share(envid_t dstenv, void *addr, int perm)
{
	int r;

	if ((perm & PTE_W) == PTE_W) {
		if ((r = sys_page_alloc(dstenv, addr, PTE_P | PTE_U | PTE_W)) < 0)
			panic("[sys_page_alloc]: %e", r);
		if ((r = sys_page_map(
		             dstenv, addr, 0, UTEMP, PTE_P | PTE_U | PTE_W)) < 0)
			panic("[sys_page_map]: %e", r);
		memmove(UTEMP, addr, PGSIZE);
		if ((r = sys_page_unmap(0, UTEMP)) < 0)
			panic("[sys_page_unmap]: %e", r);
	} else {
		if ((r = sys_page_map(dstenv, addr, 0, UTEMP, PTE_P | PTE_U)) < 0)
			panic("[sys_page_map]: %e", r);
	}
}

envid_t
fork_v0()
{
	envid_t envid;
	uint8_t *addr;
	int r;

	if ((envid = sys_exofork()) < 0)
		panic("[sys_exofork]: %e", envid);

	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	for (addr = (uint8_t *) 0; addr < (uint8_t *) UTOP; addr += PGSIZE) {
		if ((uvpd[PDX(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_P))
			dup_or_share(envid, addr, uvpt[PGNUM(addr)] & PTE_SYSCALL);
	}

	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("[sys_env_set_status]: %e", r);

	return envid;
}
//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	envid_t envid;
	uint8_t *addr;
	int r;
	extern void _pgfault_upcall(void);

	set_pgfault_handler(pgfault);

	if ((envid = sys_exofork()) < 0)
		panic("[sys_exofork]: %e", envid);

	if (envid == 0) {
		// en el hijo
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	for (addr = (uint8_t *) 0; addr < (uint8_t *) (UXSTACKTOP - PGSIZE);
	     addr += PGSIZE) {
		if ((uvpd[PDX(addr)] & PTE_P) == PTE_P) {
			if ((uvpt[PGNUM(addr)] & PTE_P) == PTE_P) {
				duppage(envid, PGNUM(addr));
			}
		}
	}

	if ((r = sys_page_alloc(
	             envid, (void *) (UXSTACKTOP - PGSIZE), PTE_SYSCALL)) < 0) {
		panic("[set_pgfault_handler]: %e", r);
	}

	if ((r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall)) < 0) {
		panic("[set_pgfault_handler]: %e", r);
	}

	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("[sys_env_set_status]: %e", r);

	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}

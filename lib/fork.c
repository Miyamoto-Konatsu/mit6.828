// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

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
	//cprintf("pgfault addr : %x, err code : %d\n",addr,err);


	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	unsigned pgnum = PGNUM(addr);
	uint32_t perm = uvpt[pgnum] & (PGSIZE - 1);

	uint32_t cow = perm & PTE_COW;
	if((err & FEC_WR) == 0 || !cow)
		panic("The page falut is not copy-on-write fault!\n,perm %d\n",perm);
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	perm = PTE_P | PTE_U | PTE_W;
	r = sys_page_alloc(0, PFTEMP, perm);
	if(r < 0)
		panic("%e\n",r);
	memmove((void *) PFTEMP, (void *) ROUNDDOWN(addr,PGSIZE), PGSIZE);
	r = sys_page_map(0,PFTEMP,0, (void *) ROUNDDOWN(addr,PGSIZE),perm);
	if(r < 0)
		panic("%e\n",r);
	r = sys_page_unmap(0,  (void *) PFTEMP);
	if(r < 0)
		panic("%e\n",r);
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

	// LAB 4: Your code here.
	bool writable = false, present = false,cow = false;

	int perm = uvpt[pn] & (PGSIZE - 1);
	writable = perm & ~PTE_W;
	present = perm & ~PTE_P;
	cow = perm & ~PTE_COW;
	if(!present)
		return 0;
	if(!writable && !cow) {
		if((r=sys_page_map(0, (void*)( pn << PGSHIFT), envid , (void*)( pn << PGSHIFT), PTE_U|PTE_P)) < 0)
		return r;
	}
	perm = PTE_P |PTE_U|PTE_COW;
	
	if((r=sys_page_map(0, (void*)( pn << PGSHIFT), envid , (void*)( pn << PGSHIFT), perm)) < 0)
		return r;
	sys_page_map(0,(void*)(pn << PGSHIFT),0 ,(void*)(pn<<PGSHIFT),perm);
	return 0;
	panic("duppage not implemented");
}

void
duppagecontent(envid_t dstenv, void *addr)
{
	int r;

	// This is NOT what you should do in your fork.
	// child首先分配一物理页，把addr映射到这个物理页
	if ((r = sys_page_alloc(dstenv, addr, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_alloc: %e", r);
	// 获得child分配的物理页，将parent的UTEMP虚拟页映射到那个物理页
	if ((r = sys_page_map(dstenv, addr, 0, UTEMP, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_map: %e", r);
	// parent把addr开始的一页数据复制到UTEMP。物理内存角度上就是把数据复制到child的那个物理页去了
	memmove(UTEMP, addr, PGSIZE);
	// parent取消UTEMP映射
	if ((r = sys_page_unmap(0, UTEMP)) < 0)
		panic("sys_page_unmap: %e", r);
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
	set_pgfault_handler(pgfault);
	envid_t envid;
	if ((envid = sys_exofork()) < 0)
		return envid;
	if(envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	extern unsigned char end[];
	int r;
	for(unsigned i = 0; i < PGNUM(ROUNDUP((uintptr_t)end, PGSIZE));){
		if(uvpd[ i >> 10] & PTE_P ) {
			if((r=duppage(envid, i++)) < 0)
				panic("error %e\n",r);
		}else{
			i+=1024;
		}
	}
	extern void _pgfault_upcall(void);
	duppagecontent(envid, (void*) USTACKTOP - PGSIZE);
	sys_page_alloc(envid,(void*) UXSTACKTOP - PGSIZE, PTE_W | PTE_U | PTE_P);
	sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
	sys_env_set_status(envid, ENV_RUNNABLE);
	return envid;
	panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}

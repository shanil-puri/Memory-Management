/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	kprintf("Creating a process with private heap\n");

	STATWORD ps;
	int pid;
	bs_map_t * bsmap;
	int* param;

	disable(ps);


	//check for free bs store
	*param = hsize;
	if (get_bsm(param) == SYSERR) {	
		restore(ps);
		return SYSERR;
	}
	
	//creates a process
	pid = create(procaddr,ssize,hsize,priority,name,nargs,args);
	
	//map bs store values
	bsmap = &bsm_tab[*param];
	bsmap->bs_status = BSM_MAPPED;
	bsmap->bs_npages = hsize;
	bsmap->bs_isheap = 1;

	if(bsm_map(pid,4096,bsmap->bsid,hsize) == SYSERR)  { 
		restore(ps);
		return SYSERR;
	}
	
	//assign process values	
	proctab[pid].store = bsmap->bsid;
	proctab[pid].vhpno = 4096;
	proctab[pid].vhpnpages = hsize;
	proctab[pid].vmemlist= (struct virtualblock*)getmem(sizeof(struct vmblock));
	
	struct vmblock * nextVmblock = getmem(sizeof(struct vmblock));
	nextVmblock->vmaddr = 4096*4096;
	nextVmblock->vmlen = hsize*4096;
	nextVmblock->vmnext = (struct vmblock*)NULL;
	proctab[pid].vmemlist->vmnext = (struct vmblock*)nextVmblock;

	//test here if page fault occurs
	
	restore(ps);
	return(pid);
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}

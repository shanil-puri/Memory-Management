/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
	struct	vmblock *block;
	unsigned size;
{
	//kprintf("In vfreemem.c \n");
	STATWORD ps;    
	struct	vmblock *p, *q;
	unsigned top;
	struct pentry * pptr;

	if (size==0 || (unsigned)block>(unsigned)maxaddr	//changes?
	    || ((unsigned)block)<((unsigned) &end))
		return(SYSERR);
	size = (unsigned)roundmb(size);
	disable(ps);
	
	pptr = &proctab[currpid];
	for( p=pptr->vmemlist->vmnext,q= &(pptr->vmemlist);
	     p != (struct vmblock *) NULL && p < block ;
	     q=p,p=p->vmnext )
		;
	if (((top=q->vmlen+(unsigned)q)>(unsigned)block && q!= &(pptr->vmemlist)) ||
	    (p!=NULL && (size+(unsigned)block) > (unsigned)p )) {
		restore(ps);
		return(SYSERR);
	}
	if ( q!= &(pptr->vmemlist) && top == (unsigned)block )
			q->vmlen += size;
	else {
		block->vmlen = size;
		block->vmnext = p;
		q->vmnext = block;
		q = block;
	}
	if ( (unsigned)( q->vmlen + (unsigned)q ) == (unsigned)p) {
		q->vmlen += p->vmlen;
		q->vmnext = p->vmnext;
	}
	restore(ps);
	return(OK);
}

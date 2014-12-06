/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{

	STATWORD ps;    
	struct	vmblock *p, *q, *leftover;
	struct pentry * pptr;

	disable(ps);
	pptr = &proctab[currpid];
	if (nbytes==0 || (pptr->vmemlist->vmnext)== (struct vmblock *) NULL) {
		restore(ps);
		return( (WORD *)SYSERR);
	}
	nbytes = (unsigned int) roundmb(nbytes);
	for (q= &(pptr->vmemlist),p=pptr->vmemlist->vmnext ;
	     p != (struct vmblock *) NULL ;
	     q=p,p=p->vmnext)
		if ( p->vmlen == nbytes) {
			q->vmnext = p->vmnext;
			restore(ps);
			return( (WORD *)p );
		} else if ( p->vmlen > nbytes ) {
			//kprintf("number of bytes reqired %d\n",nbytes);
			leftover = (struct vmblock *)( (unsigned)p + nbytes );
			q->vmnext = leftover;
			leftover->vmnext = p->vmnext;
			leftover->vmlen = p->vmlen - nbytes;
			restore(ps);
			return( (WORD *)p );
		}
	restore(ps);
	return( (WORD *)SYSERR );
}



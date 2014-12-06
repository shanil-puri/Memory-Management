/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  /* sanity check ! */

  if ( (virtpage < 4096) || ( source < 0 ) || ( source > MAX_ID) ||(npages < 1) || ( npages >128)){
	kprintf("xmmap call error: parameter error! \n");
	return SYSERR;
  }

  //kprintf("In xmmap() !\n");

  STATWORD ps;
  bs_map_t * bsmap;

  disable(ps);
  bsmap = &bsm_tab[source];
  if(bsmap->bs_status == BSM_UNMAPPED) {
	restore(ps);
	return SYSERR;
  }

  if(bsmap->bs_isheap == 1) {
	restore(ps);
	return SYSERR;
  }

  if (bsm_map(currpid,virtpage,bsmap->bsid,npages) == SYSERR) {
	restore(ps);
	return SYSERR;
  }
  //kprintf("In xmmap() : bs map created\n");

  restore(ps);
  return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage )
{
  /* sanity check ! */
  if ( (virtpage < 4096) ){ 
	kprintf("xmummap call error: virtpage (%d) invalid! \n", virtpage);
	return SYSERR;
  }

  //kprintf("In xmunmap() !");
  STATWORD ps;
  bs_map_t * bsmap;
  int store;
  int pageth;
  int pgtbl_offset;
  int flag; 
  
  disable(ps);
  
  char *vraddr = (char*)(virtpage<<12);
  
  if (bsm_lookup(currpid, vraddr, &store, &pageth) == SYSERR) {
		kprintf("back store mapping not found !\n");
		restore(ps);
		return SYSERR;
	}
	//kprintf("bsm id looked up %d\n",store);
	
	//send virtpage-pageth offset  n bsid
	pgtbl_offset = virtpage - pageth;
	release_frames(store,pgtbl_offset);

	bsm_unmap(currpid, virtpage,flag);
  restore(ps);
  return OK;
}


#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */
   
  if ( ( bs_id < 0 ) || ( bs_id > MAX_ID) ||(npages < 1) || ( npages > BACKING_STORE_UNIT_SIZE/NBPG)){
  	kprintf("get_bs call error: parameter error! \n");
  	return SYSERR;
  }

  //kprintf("In get_bs.c !\n");

  STATWORD ps;
  bs_map_t * bsmap;

  bsmap = &bsm_tab[bs_id];

  if(bsmap->bs_isheap == 1) {
  	restore(ps);
  	return SYSERR;
  }

  disable(ps);
  if (bsmap->bs_status == BSM_MAPPED) {
  	restore(ps);
  	return bsmap->bs_npages;
  }

  if(bsmap->bs_status == BSM_UNMAPPED) {
  	bsmap->bs_status = BSM_MAPPED;
  	bsmap->bs_pid = currpid;
  	bsmap->bs_npages = npages;	
  	restore(ps);
  	return npages;
  }

  restore(ps);  

  return SYSERR;

}



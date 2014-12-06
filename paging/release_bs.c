#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {

  /* release the backing store with ID bs_id */
	STATWORD ps;
	disable(ps);
	
	//bs_free(&bs_tab[bsid]);
	
	bs_map_t * bsmap;
	bs_map_t * curr;
	bsmap = &bsm_tab[bs_id];
	
	curr = bsmap->nextMap;
	if(curr != NULL) {
		//cant release BS - has maps
		//kprintf("release bs unsucessfdjkf\n");
			return OK;
	}
	
	free_bsm(bs_id);
	
	//kprintf("bs map reset \n"); 
	restore(ps);
   return OK;

}

